#include "adapters/clap/plugin.hpp"
#ifdef DRUMFOUNDRY_UI
#include "ui/workbench.hpp"
#include <stdexcept>

namespace {
void Check(bool ok) {
  if (!ok)
    throw std::runtime_error("Workbench playback/layout regression");
}
template <class T> T *Find(visage::Frame &frame) {
  if (auto *match = dynamic_cast<T *>(&frame))
    return match;
  for (auto *child : frame.children())
    if (auto *match = Find<T>(*child))
      return match;
  return nullptr;
}
} // namespace
#endif

void WorkbenchTests() {
#ifdef DRUMFOUNDRY_UI
  using namespace drumfoundry;
  using namespace clap_adapter;
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  Plugin plugin(&host);
  Check(plugin.Init());
  auto document = plugin.EditableDocument();
  document["reference"] = nullptr;
  document["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
  plugin.EditDocument(document);
  bool running = false;
  unsigned settings = 0, strikes = 0;
  ui::Bridge bridge;
  bridge.value = [&](unsigned id) { return plugin.Value(id); };
  bridge.document = [&] { return plugin.EditableDocument(); };
  bridge.audioRunning = [&] { return running; };
  bridge.settings = [&] { ++settings; };
  bridge.strike = [&](float v, float x) {
    ++strikes;
    Check(plugin.QueueStrike(v, x));
  };
  ui::Workbench editor(bridge);
  auto *pad = Find<ui::StrikePad>(editor);
  auto *analysis = Find<ui::AnalysisPanel>(editor);
  Check(pad && analysis);
  for (int width : {1000, 1440, 3200}) {
    editor.setBounds(0, 0, width, 1000);
    Check(pad->width() <= 360 && pad->height() >= 120);
    for (auto *control : analysis->children())
      if (control->isVisible())
        Check(control->x() >= 0 && control->right() <= analysis->width() + 1 &&
              control->y() >= 0 && control->bottom() <= analysis->height() + 1);
    auto *plot = Find<ui::AnalysisView>(*analysis);
    Check(plot && plot->height() >= 180);
  }
  visage::MouseEvent click;
  click.button_id = visage::kMouseButtonLeft;
  click.position = {pad->width() / 2, pad->height() / 4};
  pad->processMouseDown(click);
  Check(settings == 1 &&
        strikes == 0); // Never silently enqueue into stopped audio.
  Check(plugin.Activate(48000, 1, 128));
  plugin.processing = true;
  running = true;
  pad->processMouseDown(click);
  Check(strikes == 1);
  float left[128]{}, right[128]{};
  float *channels[]{left, right};
  clap_audio_buffer_t bus{channels, nullptr, 2, 0, 0};
  clap_process_t process{};
  process.frames_count = 128;
  process.audio_outputs = &bus;
  process.audio_outputs_count = 1;
  double energy = 0;
  for (int block = 0; block < 32; ++block) {
    Check(plugin.Process(&process) == CLAP_PROCESS_CONTINUE);
    for (float v : left)
      energy += v * v;
  }
  Check(energy > 1e-6 && plugin.PreviewStrength() == .75);
  plugin.Deactivate();
#endif
}
