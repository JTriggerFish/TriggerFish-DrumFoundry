#include "adapters/clap/plugin.hpp"
#include <stdexcept>

namespace {
using namespace drumfoundry;
using namespace clap_adapter;
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
clap_id Id(const char *key) {
  for (const auto &p : DesignParameters())
    if (p.recipe == detail::Recipe::MetallicPlate && p.descriptor->key == key)
      return p.id;
  throw std::runtime_error("Missing metallic parameter");
}
void Set(Json &d, const char *key, double value) {
  for (auto &node : d["instrument"]["nodes"])
    if (node["parameters"].contains(key)) {
      node["parameters"][key] = value;
      return;
    }
  throw std::runtime_error("Missing document parameter");
}
struct CurveEvents {
  std::array<clap_event_param_value_t, 2> events{};
  CurveEvents(double knot, double upper) {
    for (auto &e : events) {
      e.header = {sizeof(e), 0, CLAP_CORE_EVENT_SPACE_ID,
                  CLAP_EVENT_PARAM_VALUE, 0};
      e.note_id = e.port_index = e.channel = e.key = -1;
    }
    events[0].param_id = Id("body_decay_frequency_1");
    events[0].value = knot;
    events[1].param_id = Id("body_decay_frequency_7");
    events[1].value = upper;
  }
  clap_input_events_t input{
      this, [](const clap_input_events_t *) -> uint32_t { return 2; },
      [](const clap_input_events_t *in,
         uint32_t i) -> const clap_event_header_t * {
        return &static_cast<CurveEvents *>(in->ctx)->events[i].header;
      }};
};
} // namespace

void PendingDesignTests() {
  clap_host_t host{CLAP_VERSION,  nullptr, "Pending design",
                   "TriggerFish", "",      "1"};
  auto p = std::make_unique<Plugin>(&host);
  Check(p->Init(), "Initialize pending design test");
  p->SelectFactory(5);
  auto old = p->EditableDocument();
  Set(old, "body_decay_active_1", 1);
  Set(old, "body_decay_frequency_1", 10000);
  Set(old, "body_decay_frequency_7", 15000);
  p->EditDocument(old);
  p->DrainEditor(nullptr, false);
  Check(p->Activate(48000, 1, 512), "Activate old design");
  p->processing = true;

  auto next = p->EditableDocument();
  Set(next, "body_decay_frequency_1", 19000);
  Set(next, "body_decay_frequency_7", 20000);
  Set(next, "resolved_frequency_0", 301); // Force structural preparation.
  p->EditDocument(next);
  const auto wanted = p->EditableDocument();
  const auto knot = Id("body_decay_frequency_1");
  const auto upper = Id("body_decay_frequency_7");
  Check(p->RestartPending() && p->Value(upper) == 15000 &&
            p->EditorValue(upper) == 20000,
        "Active and pending designs were not kept separate");

  CurveEvents automation(18000, 18500);
  ParamsExtension.flush(&p->api, &automation.input, nullptr);
  Check(p->Value(knot) == 18000 && p->Value(upper) == 18500,
        "Old voice automation stopped during pending restart");
  Check(p->EditableDocument() == wanted && p->EditorValue(knot) == 19000,
        "Old voice automation leaked into the pending preset");
  std::string saved;
  clap_ostream_t stream{
      &saved,
      [](const clap_ostream_t *s, const void *data, uint64_t size) -> int64_t {
        static_cast<std::string *>(s->ctx)->append(
            static_cast<const char *>(data), size);
        return size;
      }};
  Check(p->Save(&stream), "Save pending design");
  Voice validated(48000, Json::parse(saved).at("document"));
  Check(validated.Document() == wanted, "Saved pending design changed");
  p->Deactivate();
  Check(p->Value(knot) == 19000 && p->Value(upper) == 20000,
        "Stopped host did not accept pending design values");
  Check(p->Activate(48000, 1, 512), "Activate pending design");
  Check(p->EditableDocument() == wanted, "Restart changed the pending preset");
  ParamsExtension.flush(&p->api, &automation.input, nullptr);
  Check(p->EditorValue(knot) == 18000 && p->EditorValue(upper) == 18500,
        "Automation did not resume on the replacement voice");
  p->Deactivate();
  // UI batches use immutable priorities from the captured starting document.
  for (bool raise : {true, false}) {
    auto edit = p->EditableDocument();
    Set(edit, "body_decay_frequency_1", raise ? 19000 : 10000);
    Set(edit, "body_decay_frequency_7", raise ? 20000 : 15000);
    Check(p->EditLiveDocument(edit), "Queue coupled UI edit");
    p->EndDesignGesture();
    std::vector<clap_id> order;
    clap_output_events_t out{
        &order,
        [](const clap_output_events_t *o, const clap_event_header_t *e) {
          if (e->type == CLAP_EVENT_PARAM_VALUE)
            static_cast<std::vector<clap_id> *>(o->ctx)->push_back(
                reinterpret_cast<const clap_event_param_value_t *>(e)
                    ->param_id);
          return true;
        }};
    ParamsExtension.flush(&p->api, nullptr, &out);
    Check(order == (raise ? std::vector<clap_id>{upper, knot}
                          : std::vector<clap_id>{knot, upper}),
          "UI coupled edit order changed");
    Check(p->EditableDocument() == edit, "UI coupled edit lost values");
  }
}
