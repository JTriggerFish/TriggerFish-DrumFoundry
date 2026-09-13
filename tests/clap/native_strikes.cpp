#include "adapters/clap/plugin.hpp"
#ifdef DRUMFOUNDRY_UI
#include "editing/routes.hpp"
#endif
#include <cmath>
#include <stdexcept>
namespace {
void Check(bool condition) {
  if (!condition)
    throw std::runtime_error("Native pad / direct Voice parity regression");
}
} // namespace
void NativeStrikeParity(const drumfoundry::Json &document) {
  using namespace drumfoundry;
  using namespace drumfoundry::clap_adapter;
  unsigned restarts = 0;
  clap_host_t host{CLAP_VERSION, &restarts, "Test", "TriggerFish", "", "1"};
  host.request_restart = [](const clap_host_t *h) {
    ++*static_cast<unsigned *>(h->host_data);
  };
  Plugin plugin(&host);
  Check(plugin.Init());
  const auto sourceStrike =
      ReadStrike(document.at("controls").at("event"), true);
  Check(plugin.Value(Hardness) == double(sourceStrike.hardness));
  Check(plugin.Value(Implement) == double(sourceStrike.implement));
  Check(plugin.Value(ContactSpread) == double(sourceStrike.contactSpread));
  plugin.EditDocument(document);
  plugin.SetParameter(Master, 0);
  plugin.SetParameter(Protection, 0);
  plugin.SetParameter(ContactSpread, .37);
  Check(plugin.Activate(48000, 1, 256));
  plugin.processing = true;
  Voice direct(48000, plugin.EditableDocument());
  float left[256]{}, right[256]{}, expected[256]{};
  float *channels[]{left, right};
  clap_audio_buffer_t bus{channels, nullptr, 2, 0, 0};
  clap_process_t process{};
  process.frames_count = 256;
  process.audio_outputs = &bus;
  process.audio_outputs_count = 1;
  for (float velocity : {.71357f, .82421f}) {
    Check(plugin.QueueStrike(velocity, .36f));
    auto strike = direct.Event();
    strike.strength = velocity;
    strike.hardness = .36f;
    direct.Trigger(strike);
    for (int block = 0; block < 8; ++block) {
#ifdef DRUMFOUNDRY_UI
      if (block == 4) {
        const auto revision = plugin.DocumentRevision();
        auto positions = editing::NodePositions(plugin.EditableDocument());
        positions.begin().value()["x"] = 80;
        plugin.EditLayout(positions);
        Check(editing::NodePositions(plugin.EditableDocument()) == positions);
        Check(plugin.DocumentRevision() == revision && restarts == 0);
      }
#endif
      Check(plugin.Process(&process) == CLAP_PROCESS_CONTINUE);
      direct.Process(expected, 256);
#ifdef DRUMFOUNDRY_UI
      float monitored[256]{};
      const auto tap = plugin.ReadVoice(monitored, 256);
      Check(tap.samples == 256 && tap.rate == 48000);
      Check(tap.firstStrike == (block == 0 ? 0u : ~0u));
      for (unsigned i = 0; i < 256; ++i)
        Check(monitored[i] == expected[i]);
#endif
      for (int i = 0; i < 256; ++i)
        Check(std::abs(left[i] - expected[i]) < 1e-7f && left[i] == right[i]);
    }
    Check(plugin.PreviewStrength() == double(velocity));
  }
  plugin.Deactivate();
}
