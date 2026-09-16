#include "adapters/clap/plugin.hpp"
#include "ui/workbench.hpp"
#include <stdexcept>

namespace {
using namespace drumfoundry;
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
} // namespace
void WorkbenchHistoryControls() {
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  auto plugin = std::make_unique<clap_adapter::Plugin>(&host);
  Check(plugin->Init(), "Control history init");
  auto document = plugin->EditableDocument();
  document["reference"] = nullptr;
  document["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
  plugin->EditDocument(document);
  ui::Bridge b;
  b.history = plugin->editHistory;
  b.value = [&](unsigned id) { return plugin->EditorValue(id); };
  b.document = [&] { return plugin->EditableDocument(); };
  b.change = [&](unsigned id, double value) {
    Check(plugin->QueueEdit(id, value), "Failed to queue test value");
  };
  b.applyDocument = [&](const auto &d) { plugin->EditDocument(d); };
  b.presentation = [&](const auto &r, const auto &a) {
    plugin->EditPresentation(r, a);
  };
  auto editor = std::make_unique<ui::Workbench>(b);
  editor->setBounds(0, 0, 1200, 800);
  ui::Slider *master{};
  ui::HelpButton *undo{}, *redo{};
  for (auto *child : editor->children()) {
    if (auto *slider = dynamic_cast<ui::Slider *>(child))
      if (slider->help.rfind("Master", 0) == 0)
        master = slider;
    if (auto *button = dynamic_cast<ui::HelpButton *>(child)) {
      if (button->help.rfind("Undo", 0) == 0)
        undo = button;
      if (button->help.rfind("Redo", 0) == 0)
        redo = button;
    }
  }
  Check(master && undo && redo, "History controls missing");
  Check(!undo->isActive() && !redo->isActive(),
        "Empty history buttons enabled");
  Check(master->SubmitText("-18"), "Master text entry failed");
  Check(master->SubmitText("-24"), "Consecutive queued edit failed");
  editor->Undo();
  Check(plugin->EditorValue(clap_adapter::Master) == -18 &&
            plugin->Value(clap_adapter::Master) == -12,
        "Undo must restore the preceding requested value before audio catches "
        "up");
  editor->Redo();
  Check(plugin->EditorValue(clap_adapter::Master) == -24,
        "Queued redo must restore the second gesture");
  editor->Undo();
  Check(undo->isActive() && redo->isActive(), "History buttons did not update");
  undo->onToggle().callback(undo, false);
  redo->onToggle().callback(redo, false);
  // No callback has consumed the edit, undo or redo yet. Last queued value
  // wins.
  clap_adapter::ParamsExtension.flush(&plugin->api, nullptr, nullptr);
  Check(plugin->Value(clap_adapter::Master) == -18,
        "Fast redo was lost because audio readback was stale");
  editor->Undo();
  clap_adapter::ParamsExtension.flush(&plugin->api, nullptr, nullptr);
  Check(plugin->Value(clap_adapter::Master) == -12, "Master undo failed");
  plugin->SetParameter(clap_adapter::Master, -9);
  Check(plugin->EditorValue(clap_adapter::Master) == -9,
        "Acknowledged edits must not hide later host automation");
  Check(master->SubmitText("-24"), "Second master edit failed");
  Check(!b.history->CanRedo(), "Editing after undo retained old redo");
  visage::TextEditor text;
  editor->addChild(&text);
  text.processFocusChanged(true, false);
#ifdef __APPLE__
  const int modifier = visage::kModifierCmd;
#else
  const int modifier = visage::kModifierRegCtrl;
#endif
  editor->keyPress({visage::KeyCode::Z, modifier, true});
  Check(b.history->CanUndo() && !b.history->CanRedo(),
        "Empty text undo must not fall through to instrument undo");
  text.processFocusChanged(false, false);
  editor->removeChild(&text);
  editor->Undo();
  Check(b.history->CanRedo(),
        "Instrument undo did not resume after text focus");
  // Host automation does not become an editor gesture, and externally selected
  // instruments must not inherit stale patches from a different recipe.
  plugin->SetParameter(clap_adapter::Preset, 5);
  plugin->PrepareEditorPreset();
  Check(!b.history->CanUndo() && !b.history->CanRedo(),
        "Host preset retained stale history");
}
