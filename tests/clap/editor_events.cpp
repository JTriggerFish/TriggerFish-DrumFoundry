#include "adapters/clap/plugin.hpp"
#include <cmath>
#include <fstream>
#include <stdexcept>

using namespace drumfoundry::clap_adapter;
void NativeStrikeParity(const drumfoundry::Json &);
void Require(bool result) {
  if (!result)
    throw std::runtime_error("CLAP editor event regression");
}
int main(int argc, char **argv) {
  extern void DocumentHostTests();
  DocumentHostTests();
  extern void WorkbenchTests();
  WorkbenchTests();
  Require(argc == 2);
  std::ifstream source(argv[1]);
  NativeStrikeParity(drumfoundry::Json::parse(source));
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  Plugin presets(&host);
  Require(presets.Init());
  Require(presets.EditableDocument().at("reference").is_null());
  for (unsigned i = 0; i < 6; ++i) {
    presets.SelectFactory(i);
    const auto original = presets.EditableDocument();
    presets.EditPresentation({{"libraryPath", "test/kick.wav"}},
                             original.at("controls").at("analysis"));
    Require(
        presets.EditableDocument().at("reference").contains("libraryPath"));
    presets.SelectFactory(i);
    const auto factory = presets.EditableDocument();
    Require(factory.at("reference").is_null());
    Require(factory.at("instrument") == original.at("instrument"));
    Require(factory.at("controls").at("event") ==
            original.at("controls").at("event"));
  }
  Plugin plugin(&host);
  Require(plugin.Init());
  auto document = plugin.EditableDocument();
  auto analysis = document.at("controls").at("analysis");
  analysis["view"] = {{"span", 3.5}, {"pan", .1}};
  const auto revision = plugin.DocumentRevision();
  plugin.EditPresentation({{"id", "fixture"}, {"sha256", "test"}}, analysis);
  Require(plugin.DocumentRevision() == revision);
  Require(plugin.EditableDocument().at("reference").at("id") == "fixture");
  Require(plugin.EditableDocument().at("controls").at("analysis") ==
          analysis);
  Require(!plugin.QueueEdit(Master, 12));
  Require(plugin.QueueEdit(Master, -20));
  Require(plugin.Value(Master) ==
          -12); // Producer must not mutate the audio state.
  unsigned sent = 0;
  clap_output_events_t out{
      &sent,
      [](const clap_output_events_t *out, const clap_event_header_t *event) {
        if (event->space_id != CLAP_CORE_EVENT_SPACE_ID || event->time)
          return false;
        ++*static_cast<unsigned *>(out->ctx);
        return true;
      }};
  ParamsExtension.flush(&plugin.api, nullptr, &out);
  Require(plugin.Value(Master) == -20 && sent == 3);
  Require(plugin.QueueStrike(.75f, .5f));
  ParamsExtension.flush(&plugin.api, nullptr,
                        &out); // Strike survives inactive flush.
  Require(plugin.Activate(48000, 1, 128));
  plugin.processing = true;
  float left[128]{}, right[128]{};
  float *channels[]{left, right};
  clap_audio_buffer_t bus{channels, nullptr, 2, 0, 0};
  clap_process_t process{};
  process.frames_count = 128;
  process.audio_outputs = &bus;
  process.audio_outputs_count = 1;
  double energy = 0;
  for (unsigned i = 0; i < 32; ++i) {
    Require(plugin.Process(&process) != CLAP_PROCESS_ERROR);
    for (float value : left) {
      Require(std::isfinite(value));
      energy += value * value;
    }
  }
  Require(energy > 0 && !plugin.EditorErrors());
  Require(plugin.QueuePanic());
  Require(plugin.Process(&process) != CLAP_PROCESS_ERROR);
  for (float value : left)
    Require(value == 0);
  plugin.processing = false;
  plugin.Deactivate();
  const auto before = plugin.EditableDocument();
  auto edited = before;
  edited["controls"]["event"]["hardness"] = .25;
  plugin.EditDocument(edited);
  Require(plugin.DocumentRevision() == 1 && plugin.Value(Hardness) == .25);
  Require(plugin.EditableDocument() == edited);
  bool rejected = false;
  try {
    plugin.EditDocument({{"schema", "invalid"}});
  } catch (...) {
    rejected = true;
  }
  Require(rejected && plugin.EditableDocument() == edited);
  plugin.SetParameter(Preset, 5);
  plugin.PrepareEditorPreset();
  Require(std::abs(plugin.Value(Hardness) - .35) < 1e-6);
  Require(std::abs(plugin.Value(Location) - .536997846523398) < 1e-6);
  return 0;
}
