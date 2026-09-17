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
visage::Frame *Named(visage::Frame &frame, const std::string &name) {
  if (frame.name() == name) return &frame;
  for (auto *child : frame.children())
    if (auto *found = Named(*child, name)) return found;
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
  extern void WorkbenchHistoryTests();
  WorkbenchHistoryTests();
  extern void WorkbenchLiveTests();
  WorkbenchLiveTests();
  extern void WorkbenchModuleTests();
  WorkbenchModuleTests();
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
  float lastVelocity = 0, lastPosition = 0;
  ui::Bridge bridge;
  bridge.value = [&](unsigned id) { return plugin.Value(id); };
  bridge.document = [&] { return plugin.EditableDocument(); };
  bridge.audioRunning = [&] { return running; };
  bridge.settings = [&] { ++settings; };
  bridge.strike = [&](float v, float x) {
    ++strikes;
    lastVelocity = v;
    lastPosition = x;
    Check(plugin.QueueStrike(v, x));
  };
  ui::Workbench editor(bridge);
  auto *pad = Find<ui::StrikePad>(editor);
  auto *analysis = Find<ui::AnalysisPanel>(editor);
  Check(pad && analysis);
  auto *referencePlay = Find<ui::PlayButton>(*analysis);
  Check(referencePlay && !referencePlay->isVisible());
  editor.setBounds(0, 0, 900, 600);
  editor.OpenLayout();
  auto *layout = Find<ui::LayoutPanel>(editor);
  Check(layout && layout->parent()->isVisible());
  auto *toggle = dynamic_cast<visage::UiButton *>(layout->children().front());
  Check(toggle);
  toggle->onToggle().callback(toggle, false);
  Check(!analysis->showSpectrogram && layout->parent()->isVisible());
  toggle->onToggle().callback(toggle, false);
  Check(analysis->showSpectrogram);
  for (int size : {0, 1, 2}) {
    layout->selected(4 + size);
    Check(analysis->textSize == size);
    Check(ui::FrameFont(*pad).size() == 13 + 2 * size);
    Check(analysis->Settings().at("view").at("textSize") == size);
    CheckChildren(*layout);
  }
  layout->selected(3);
  Check(analysis->textSize == 0 && analysis->showSpectrogram &&
        analysis->showModalEditor);
  layout->close();
  Check(!layout->parent()->isVisible());
  extern void WorkbenchSplitTests(ui::Workbench &, ui::AnalysisPanel &);
  WorkbenchSplitTests(editor, *analysis);
  for (auto size : {std::array<int, 2>{900, 600},
                    {1024, 768},
                    {1280, 720},
                    {1440, 900},
                    {3200, 1800}}) {
    editor.setBounds(0, 0, size[0], size[1]);
    for (float controlWidth : {300.f, 460.f, 700.f}) {
      editor.SetControlWidth(controlWidth);
      Check(pad->width() <= 360 && pad->height() >= 120);
      auto *playing = Named(editor, "playing-controls");
      auto *freezeButton = Named(editor, "freeze-strike");
      Check(playing && freezeButton && playing->parent() == pad->parent());
      Check(freezeButton->y() >= pad->bottom());
      if (playing->isVisible()) {
        Check(playing->right() <= pad->parent()->width());
        Check(playing->x() >= pad->right() || playing->y() >= freezeButton->bottom());
        auto *modes = Find<ui::ModalPanel>(editor);
        Check(modes && modes->y() >= playing->bottom());
      }
      CheckChildren(editor);
      CheckChildren(*analysis);
      auto *plot = Find<ui::AnalysisView>(*analysis);
      Check(plot && plot->height() >= 180);
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
  const auto playable = pad->PlayingBounds();
  click.position = {playable.x() + playable.width() / 2,
                    playable.y() + playable.height() / 4};
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
  auto *freezeButton = dynamic_cast<visage::UiButton *>(Named(editor, "freeze-strike"));
  auto *freeze = Find<ui::StrikeFreeze>(editor);
  Check(freezeButton && freeze);
  freezeButton->onToggle().callback(freezeButton, false);
  Check(freeze->parent()->isVisible());
  auto *enabled = dynamic_cast<visage::UiButton *>(Named(*freeze, "freeze-enabled"));
  auto *velocity = dynamic_cast<ui::Slider *>(Named(*freeze, "frozen-velocity"));
  auto *position = dynamic_cast<ui::Slider *>(Named(*freeze, "frozen-position"));
  Check(enabled && velocity && position);
  Check(velocity->SubmitText("0.65") && position->SubmitText("0.35"));
  enabled->onToggle().callback(enabled, false);
  freeze->close();
  pad->strike(.1f, .9f);
  Check(lastVelocity == .65f && lastPosition == .35f);
  pad->strike(.9f, .1f);
  Check(lastVelocity == .65f && lastPosition == .35f);
  // Reopening keeps the frozen values, independent of later performance input.
  freeze->Open(.2f, .8f, false);
  Check(freeze->Apply(.9f, .1f) == std::pair(.65f, .35f));
  enabled->onToggle().callback(enabled, false);
  pad->strike(.9f, .1f);
  Check(lastVelocity == .9f && lastPosition == .1f);
  enabled->onToggle().callback(enabled, false);
  freeze->SetKick(true);
  Check(!freeze->Enabled());
  plugin.Deactivate();
  for (unsigned preset = 0; preset < 6; ++preset) {
    plugin.SelectFactory(preset);
    auto smallDocument = plugin.EditableDocument();
    smallDocument["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
    plugin.EditDocument(smallDocument);
    const auto sound = plugin.EditableDocument().at("instrument");
    ui::Workbench small(bridge);
    auto *spread = dynamic_cast<ui::Slider *>(Named(small, "contact-spread"));
    Check(spread);
    Check(spread->Enabled() ==
          (smallDocument.at("instrument").at("recipe") != "metal.cymbal.v1" ||
           plugin.Value(102) < .5));
    small.setBounds(0, 0, 900, 600);
    small.SetControlWidth(340);
    auto *modes = Find<ui::ModalPanel>(small);
    Check(modes);
    for (int textSize : {0, 1, 2}) {
      small.SetTextSize(textSize);
      unsigned headerButtons = 0;
      float previousRight = 0;
      std::vector<visage::Frame *> header;
      for (auto *child : small.children())
        if (child->isVisible() && child->y() < 58 && child->height() <= 44)
          header.push_back(child);
      std::sort(header.begin(), header.end(),
                [](auto *a, auto *b) { return a->x() < b->x(); });
      for (auto *child : header) {
        Check(child->x() >= previousRight && child->bottom() < 58);
        previousRight = child->right();
        if (dynamic_cast<visage::UiButton *>(child))
          ++headerButtons;
      }
      Check(headerButtons == 5); // Preset, undo/redo, limiter, Settings.
      Check(small.paletteValue(ui::TextScale) == ui::TextSizeScale(textSize));
      for (bool spectrum : {true, false})
        for (bool visible : {true, false}) {
          small.SetVisualPanels(spectrum, visible);
          CheckChildren(small);
          CheckChildren(*Find<ui::AnalysisPanel>(small));
          if (modes->isVisible())
            CheckChildren(*modes);
        }
    }
    // Text preferences are instance-local and never touch synthesis controls.
    Check(editor.paletteValue(ui::TextScale) == 1);
    Check(plugin.EditableDocument().at("instrument") == sound);
  }
#endif
}
