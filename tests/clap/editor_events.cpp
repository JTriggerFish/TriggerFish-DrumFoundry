#include "adapters/clap/plugin.hpp"
#include <cmath>
#include <stdexcept>

using namespace drumfoundry::clap_adapter;
void Require(bool result) {
  if (!result)
    throw std::runtime_error("CLAP editor event regression");
}
int main() {
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  Plugin plugin(&host);
  Require(plugin.Init());
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
  return 0;
}
