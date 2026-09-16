#include "adapters/clap/plugin.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
using namespace drumfoundry;
using namespace clap_adapter;
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
clap_id Id(detail::Recipe recipe, const char *key) {
  for (const auto &p : DesignParameters())
    if (p.recipe == recipe && p.descriptor->key == key)
      return p.id;
  throw std::runtime_error(std::string("Missing automation: ") + key);
}
clap_event_param_value_t Value(clap_id id, double value, unsigned time = 0) {
  clap_event_param_value_t e{};
  e.header = {sizeof(e), time, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE,
              0};
  e.param_id = id;
  e.value = value;
  e.note_id = e.port_index = e.channel = e.key = -1;
  return e;
}
struct Input {
  std::vector<const clap_event_header_t *> events;
  clap_input_events_t api{this,
                          [](const clap_input_events_t *e) {
                            return uint32_t(
                                static_cast<Input *>(e->ctx)->events.size());
                          },
                          [](const clap_input_events_t *e, uint32_t i) {
                            return static_cast<Input *>(e->ctx)->events.at(i);
                          }};
};
struct Fixture {
  clap_host_t host{CLAP_VERSION,  nullptr, "Automation test",
                   "TriggerFish", "",      "1"};
  Plugin plugin{&host};
  std::array<float, 512> left{}, right{};
  Fixture(unsigned preset) {
    Check(plugin.Init(), "Initialize automation test");
    plugin.SelectFactory(preset);
    plugin.SetParameter(Master, 0);
    plugin.SetParameter(Protection, 0);
    Check(plugin.Activate(48000, 1, 512), "Activate automation test");
    plugin.processing = true;
  }
  ~Fixture() { plugin.Deactivate(); }
  void Render(unsigned frames = 512, Input *input = nullptr) {
    float *channels[]{left.data(), right.data()};
    clap_audio_buffer_t bus{channels, nullptr, 2, 0, 0};
    clap_process_t p{};
    p.frames_count = frames;
    p.audio_outputs = &bus;
    p.audio_outputs_count = 1;
    p.in_events = input ? &input->api : nullptr;
    Check(plugin.Process(&p) == CLAP_PROCESS_CONTINUE, "Process automation");
  }
};

void TimedAutomation() {
  auto a = std::make_unique<Fixture>(5), b = std::make_unique<Fixture>(5);
  const auto enabled = Id(detail::Recipe::MetallicPlate, "output_eq_enabled");
  a->plugin.SetParameter(enabled, 1);
  b->plugin.SetParameter(enabled, 1);
  Check(a->plugin.QueueStrike(.8f, .5f) && b->plugin.QueueStrike(.8f, .5f),
        "Initial strike");
  a->Render();
  b->Render();
  const auto id = Id(detail::Recipe::MetallicPlate, "output_high_cut");
  Check(id == 1498473345u, "Published automation ID changed");
  auto change = Value(id, 1200, 173);
  Input input;
  input.events = {&change.header};
  a->Render(512, &input);
  b->Render(173);
  for (unsigned i = 0; i < 173; ++i)
    Check(a->left[i] == b->left[i], "Automation applied too early");
  b->plugin.SetParameter(id, 1200);
  b->Render(339);
  for (unsigned i = 0; i < 339; ++i)
    Check(a->left[i + 173] == b->left[i], "Automation not sample timed");
  Check(!a->plugin.RestartPending(), "Automation requested restart");
  auto invalid = Value(id, 1e30);
  a->plugin.Event(&invalid.header);
  Check(a->plugin.Value(id) == 1200, "Invalid automation changed control");
  // Every live ID is visible only for its recipe, but remains registered
  // always.
  for (unsigned i = 0; i < DesignParameters().size(); ++i) {
    clap_param_info_t info{};
    Check(ParamsExtension.get_info(&a->plugin.api, ParameterCount + i, &info),
          "Design metadata");
    Check(bool(info.flags & CLAP_PARAM_IS_HIDDEN) ==
              (DesignParameters()[i].recipe != detail::Recipe::MetallicPlate),
          "Recipe visibility");
    char text[256]{};
    double parsed = 0;
    Check(ParamsExtension.value_to_text(&a->plugin.api, info.id,
                                        info.default_value, text,
                                        sizeof(text)) &&
              ParamsExtension.text_to_value(&a->plugin.api, info.id, text,
                                            &parsed) &&
              std::abs(parsed - info.default_value) <
                  1e-5 * (1 + std::abs(info.default_value)),
          "Design parameter text roundtrip");
  }
}

void GestureAndState() {
  auto f = std::make_unique<Fixture>(0);
  auto &p = f->plugin;
  const auto id = Id(detail::Recipe::Kick, "output_colour_gain");
  std::vector<unsigned> types;
  clap_output_events_t out{
      &types, [](const clap_output_events_t *o, const clap_event_header_t *e) {
        static_cast<std::vector<unsigned> *>(o->ctx)->push_back(e->type);
        return true;
      }};
  for (double v : {2., 3., 4.}) {
    Check(p.QueueEdit(id, v), "Queue drag value");
    ParamsExtension.flush(&p.api, nullptr, &out);
  }
  p.EndDesignGesture();
  ParamsExtension.flush(&p.api, nullptr, &out);
  Check(types == std::vector<unsigned>{CLAP_EVENT_PARAM_GESTURE_BEGIN,
                                       CLAP_EVENT_PARAM_VALUE,
                                       CLAP_EVENT_PARAM_VALUE,
                                       CLAP_EVENT_PARAM_VALUE,
                                       CLAP_EVENT_PARAM_GESTURE_END},
        "Drag must be one host gesture");
  std::string saved;
  clap_ostream_t output{
      &saved,
      [](const clap_ostream_t *s, const void *data, uint64_t n) -> int64_t {
        static_cast<std::string *>(s->ctx)->append(
            static_cast<const char *>(data), std::size_t(n));
        return n;
      }};
  Check(p.Save(&output), "Save automated state");
  p.SetParameter(id, 9);
  std::size_t offset = 0;
  struct Reader {
    std::string &data;
    std::size_t &offset;
  } reader{saved, offset};
  clap_istream_t input{
      &reader, [](const clap_istream_t *s, void *data, uint64_t n) -> int64_t {
        auto &r = *static_cast<Reader *>(s->ctx);
        n = std::min<uint64_t>(n, r.data.size() - r.offset);
        std::memcpy(data, r.data.data() + r.offset, n);
        r.offset += n;
        return n;
      }};
  Check(p.Load(&input) && p.EditorValue(id) == 4 && p.Value(id) == 9,
        "Automated value not restored from project");
  Check(!p.QueueEdit(id, 8), "Pending preset accepted edits for the old voice");
  p.Deactivate();
  Check(p.Activate(48000, 1, 512) && p.Value(id) == 4,
        "Restored design was not accepted at restart");
  p.processing = true;
  // Pending edits must not overwrite another preset after preparation.
  Check(p.QueueEdit(id, 8), "Queue before replacement");
  p.SelectFactory(0);
  const auto factory = p.EditableDocument();
  p.Deactivate();
  Check(p.Activate(44100, 1, 512), "Restore after rate change");
  p.processing = true;
  f->Render();
  Check(p.EditableDocument() == factory,
        "Stale design event crossed preset reload");
}

void InactiveCurveValidation() {
  auto f = std::make_unique<Fixture>(5);
  f->plugin.Deactivate();
  auto &p = f->plugin;
  const auto kind = detail::Recipe::MetallicPlate;
  p.SetParameter(Id(kind, "body_decay_active_1"), 0);
  p.SetParameter(Id(kind, "body_decay_frequency_7"), 20000);
  p.SetParameter(Id(kind, "body_decay_frequency_1"), 19000);
  p.SetParameter(Id(kind, "body_decay_active_1"), 1);
  p.SetParameter(Id(kind, "body_decay_frequency_7"), 15000);
  Check(p.Value(Id(kind, "body_decay_frequency_7")) == 20000,
        "Inactive automation accepted invalid T60 endpoints");
  Voice validated(48000, p.EditableDocument());
}

void CurveEventOrder() {
  for (bool active : {false, true})
    for (bool reversed : {false, true}) {
      auto f = std::make_unique<Fixture>(5);
      auto &p = f->plugin;
      if (!active)
        p.Deactivate();
      const auto kind = detail::Recipe::MetallicPlate;
      const auto knot = Id(kind, "body_decay_frequency_1");
      const auto upper = Id(kind, "body_decay_frequency_7");
      p.SetParameter(Id(kind, "body_decay_active_1"), 0);
      p.SetParameter(knot, 10000);
      p.SetParameter(upper, 15000);
      p.SetParameter(Id(kind, "body_decay_active_1"), 1);
      for (bool raise : {true, false}) {
        auto a = Value(knot, raise ? 19000 : 10000, 73);
        auto b = Value(upper, raise ? 20000 : 15000, 73);
        Input input;
        input.events =
            reversed
                ? std::vector<const clap_event_header_t *>{&b.header, &a.header}
                : std::vector<const clap_event_header_t *>{&a.header,
                                                           &b.header};
        if (active)
          f->Render(128, &input);
        else
          ParamsExtension.flush(&p.api, &input.api, nullptr);
        Check(p.Value(knot) == a.value && p.Value(upper) == b.value,
              "Same-time T60 automation depended on event order");
        Voice validated(48000, p.EditableDocument());
      }
      // Reject the coupled curve atomically, preserving unrelated EQ edits.
      auto a = Value(knot, 19000);
      auto b = Value(Id(kind, "output_colour_gain"), 3);
      Input invalid;
      invalid.events = {&a.header, &b.header};
      ParamsExtension.flush(&p.api, &invalid.api, nullptr);
      Check(p.Value(knot) == 10000 && p.Value(b.param_id) == 3,
            "Invalid curve batch corrupted controls or rejected unrelated EQ");
    }
}
} // namespace

void DesignAutomationTests() {
  TimedAutomation();
  GestureAndState();
  InactiveCurveValidation();
  CurveEventOrder();
  extern void DesignSnapshotTests();
  DesignSnapshotTests();
  extern void PendingDesignTests();
  PendingDesignTests();
}
