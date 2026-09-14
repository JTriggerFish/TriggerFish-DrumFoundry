#include "adapters/clap/plugin.hpp"
#ifdef DRUMFOUNDRY_UI
#include "ui/workbench.hpp"
#include <array>
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
void CheckChildren(visage::Frame &frame) {
  for (auto *child : frame.children())
    if (child->isVisible())
      Check(child->x() >= 0 && child->y() >= 0 && child->width() > 0 &&
            child->height() > 0 && child->right() <= frame.width() + 1 &&
            child->bottom() <= frame.height() + 1);
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
  for (auto size : {std::array<int, 2>{900, 600},
                    {1024, 768},
                    {1280, 720},
                    {1440, 900},
                    {3200, 1800}}) {
    editor.setBounds(0, 0, size[0], size[1]);
    for (float controlWidth : {300.f, 460.f, 700.f}) {
      editor.SetControlWidth(controlWidth);
      Check(pad->width() <= 360 && pad->height() >= 120);
      CheckChildren(editor);
      CheckChildren(*analysis);
      auto *plot = Find<ui::AnalysisView>(*analysis);
      Check(plot && plot->height() >= 300);
      for (bool modes : {false, true}) {
        editor.SetVisualPanels(false, modes);
        Check(!plot->isVisible());
        CheckChildren(*analysis); // Reference picker remains accessible.
        editor.SetVisualPanels(true, modes);
        Check(plot->isVisible());
        CheckChildren(*analysis);
      }
    }
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
  for (unsigned preset = 1; preset < 6; ++preset) {
    plugin.SelectFactory(preset);
    auto smallDocument = plugin.EditableDocument();
    smallDocument["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
    plugin.EditDocument(smallDocument);
    const auto sound = plugin.EditableDocument().at("instrument");
    ui::Workbench small(bridge);
    small.setBounds(0, 0, 900, 600);
    small.SetControlWidth(340);
    auto *modes = Find<ui::ModalPanel>(small);
    Check(modes);
    for (bool spectrum : {true, false})
      for (bool visible : {true, false}) {
        small.SetVisualPanels(spectrum, visible);
        CheckChildren(small);
        CheckChildren(*Find<ui::AnalysisPanel>(small));
        if (modes->isVisible())
          CheckChildren(*modes);
      }
    Check(plugin.EditableDocument().at("instrument") == sound);
  }
#endif
}
