#include "adapters/clap/plugin.hpp"
#include <cmath>
#include <stdexcept>

namespace {
using namespace drumfoundry;
using namespace clap_adapter;
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
struct Harness {
  unsigned restarts{};
  clap_host_t host{CLAP_VERSION, this, "Live test", "TriggerFish", "", "1"};
  Plugin plugin{&host};
  std::array<float, 128> left{}, right{};
  float *channels[2]{left.data(), right.data()};
  clap_audio_buffer_t bus{channels, nullptr, 2, 0, 0};
  clap_process_t process{};
  Harness(unsigned preset) {
    host.request_restart = [](const clap_host_t *h) {
      ++static_cast<Harness *>(h->host_data)->restarts;
    };
    Check(plugin.Init(), "Live test initialization");
    plugin.SelectFactory(preset);
    plugin.SetParameter(Master, 0);
    plugin.SetParameter(Protection, 0);
    Check(plugin.Activate(48000, 1, 128), "Live test activation");
    plugin.processing = true;
    process.frames_count = 128;
    process.audio_outputs = &bus;
    process.audio_outputs_count = 1;
  }
  ~Harness() { plugin.Deactivate(); }
  double Block() {
    Check(plugin.Process(&process) == CLAP_PROCESS_CONTINUE,
          "Live process failed");
    double energy = 0;
    for (float v : left) {
      Check(std::isfinite(v), "Non-finite live output");
      energy += v * v;
    }
    return energy;
  }
  bool Set(const char *key, double value) {
    auto d = plugin.EditableDocument();
    for (auto &node : d["instrument"]["nodes"])
      if (node["parameters"].contains(key)) {
        node["parameters"][key] = value;
        return plugin.EditLiveDocument(d);
      }
    throw std::runtime_error(std::string("Missing live parameter: ") + key);
  }
};

void TailAndRetrigger(unsigned preset) {
  auto a = std::make_unique<Harness>(preset);
  auto b = std::make_unique<Harness>(preset);
  Check(a->plugin.QueueStrike(.8f, .5f) && b->plugin.QueueStrike(.8f, .5f),
        "Queue strike");
  for (int i = 0; i < 16; ++i) {
    a->Block();
    b->Block();
  }
  // Final-level edits should become a constant ratio without replacing state.
  double original = 0;
  const auto initial = a->plugin.EditableDocument();
  for (const auto &node : initial.at("instrument").at("nodes"))
    if (node.at("parameters").contains("model_level_db"))
      original = node.at("parameters").at("model_level_db");
  Check(a->Set("model_level_db", original - 6), "Model gain must be live");
  for (int i = 0; i < 4; ++i) {
    a->Block();
    b->Block();
  }
  double residual = 0, energy = 0;
  for (int i = 0; i < 16; ++i) {
    a->Block();
    b->Block();
    for (int j = 0; j < 128; ++j) {
      const double expected = b->left[j] * std::pow(10., -.3);
      residual += std::pow(a->left[j] - expected, 2);
      energy += expected * expected;
    }
  }
  Check(energy > 1e-18 && residual < energy * 1e-8,
        "Live gain reset or changed the sounding voice");
  Check(a->Set("output_eq_enabled", 1), "Bypass must be live");
  Check(a->Set("output_high_cut", 500), "EQ must be live");
  for (int i = 0; i < 4; ++i)
    a->Block();
  Check(!a->restarts && !a->plugin.RestartPending(),
        "Live edits requested host restart");
  // Coalesced drags retain their final value, including a decay edit followed
  // by EQ.
  for (int i = 0; i < 40; ++i)
    Check(a->Set("output_colour_gain", i % 2 ? 12 : -12),
          "Rapid EQ edit failed");
  a->Block();
  const auto saved = a->plugin.EditableDocument();
  a->plugin.EditDocument(saved); // Mouse-up should be an audio no-op.
  Check(!a->restarts, "Commit restarted a live edit");
  a->plugin.QueuePanic();
  a->Block();
  a->plugin.QueueStrike(.8f, .5f);
  double newEnergy = 0;
  for (int i = 0; i < 16; ++i)
    newEnergy += a->Block();
  Check(newEnergy > 1e-14, "A live edit prevented retriggering");
  auto bad = saved;
  for (auto &node : bad["instrument"]["nodes"])
    if (node["parameters"].contains("output_colour_gain"))
      node["parameters"]["output_colour_gain"] = 1000;
  bool rejected = false;
  try {
    a->plugin.EditLiveDocument(bad);
  } catch (...) {
    rejected = true;
  }
  Check(rejected, "Out-of-range live edit was accepted");
  Check(a->plugin.EditableDocument() == saved,
        "Failed edit changed desired state");
}
} // namespace

void LiveControlsTests() {
  extern void LiveParameterParity();
  LiveParameterParity();
  for (unsigned preset = 0; preset < 6; ++preset)
    TailAndRetrigger(preset);
  auto h = std::make_unique<Harness>(5);
  Check(h->Set("body_decay_seconds_0", 2), "Decay curve must be live");
  Check(h->Set("output_colour_gain", 3), "EQ following decay must be live");
  Check(h->Set("bloom_rate", 4), "Bloom must be live");
  Check(!h->Set("field_satellite_density", .123),
        "Allocation edit incorrectly classified live");
  h->plugin.QueueStrike(.8f, .5f);
  double energy = 0;
  for (int i = 0; i < 64; ++i)
    energy += h->Block();
  Check(energy > 0 && !h->restarts, "Coalesced live edits failed");
  h->plugin.Deactivate();
  h->plugin.SelectFactory(0);
  Check(h->plugin.Activate(44100, 1, 128), "Reactivation failed");
  h->plugin.processing = true;
  h->Block();
  for (float v : h->left)
    Check(v == 0, "Stale live controls crossed preset/rate restart");
}
