#include "adapters/clap/plugin.hpp"
#include "ui/workbench.hpp"
#include <stdexcept>

namespace {
using namespace drumfoundry;
void Check(bool ok, const char *text) {
  if (!ok)
    throw std::runtime_error(text);
}
template <class T> T *Find(visage::Frame &frame) {
  if (auto *match = dynamic_cast<T *>(&frame))
    return match;
  for (auto *child : frame.children())
    if (auto *match = Find<T>(*child))
      return match;
  return nullptr;
}
void PatchHistory() {
  ui::EditHistory h;
  using J = nlohmann::json;
  J a{{"level", 0}, {"velocity", .5}}, b = a;
  b["level"] = 1;
  h.Record(a, b);
  b["velocity"] = .9;
  auto restored = h.Target(b, false);
  Check(restored["level"] == 0 && restored["velocity"] == .9,
        "Undo must preserve unrelated live values");
  h.Accept(false);
  Check(h.CanRedo() && !h.CanUndo(), "History cursor failed");
  h.Record(restored, restored);
  Check(h.CanRedo(), "No-op must not discard redo");
  auto branch = restored;
  branch["level"] = 2;
  h.Record(restored, branch);
  Check(!h.CanRedo(), "New edits must discard redo");
  auto compensated = branch;
  compensated["level"] = 3;
  h.Record(branch, compensated, true);
  Check(h.Target(compensated, false) == restored,
        "Automatic compensation must merge with its initiating edit");
  h.Clear();
  J current{{"level", 0}};
  for (int i = 1; i <= 140; ++i) {
    J next{{"level", i}};
    h.Record(current, next);
    current = next;
  }
  unsigned count = 0;
  while (h.CanUndo()) {
    current = h.Target(current, false);
    h.Accept(false);
    ++count;
  }
  Check(count == 128 && current["level"] == 12, "History must be bounded");
}
} // namespace
void WorkbenchHistoryTests() {
  PatchHistory();
  extern void WorkbenchHistoryControls();
  WorkbenchHistoryControls();
  extern void WorkbenchHistoryPresets();
  WorkbenchHistoryPresets();
  clap_host_t host{CLAP_VERSION, nullptr, "Test", "TriggerFish", "", "1"};
  auto ownedPlugin = std::make_unique<clap_adapter::Plugin>(&host);
  auto &plugin = *ownedPlugin;
  Check(plugin.Init(), "History test plugin init failed");
  plugin.SelectFactory(5);
  auto initial = plugin.EditableDocument();
  initial["reference"] = nullptr;
  initial["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
  plugin.EditDocument(initial);
  ui::Bridge b;
  b.history = plugin.editHistory;
  b.document = [&] { return plugin.EditableDocument(); };
  b.value = [&](unsigned id) { return plugin.Value(id); };
  b.change = [&](unsigned id, double value) { plugin.SetParameter(id, value); };
  b.velocity = [&] { return plugin.PreviewStrength(); };
  b.setVelocity = [&](double value) { plugin.SetPreviewStrength(value); };
  bool reject = false;
  b.applyDocument = [&](const auto &document) {
    if (reject)
      throw std::runtime_error("Injected host failure");
    plugin.EditDocument(document);
  };
  {
    auto ownedEditor = std::make_unique<ui::Workbench>(b);
    auto &editor = *ownedEditor;
    editor.setBounds(0, 0, 1200, 800);
    auto *plot = Find<ui::ModalPlot>(editor);
    Check(plot, "Missing modal plot");
    const auto before = plugin.EditableDocument();
    visage::MouseEvent e;
    e.button_id = visage::kMouseButtonLeft;
    e.repeat_click_count = 1;
    e.position = {plot->width() * .5f, plot->height() * .5f};
    plot->tool = ui::ModalPlot::Tool::Paint;
    plot->mouseDown(e);
    for (int i = 0; i < 5; ++i) {
      e.position.x += 4;
      plot->mouseDrag(e);
    }
    plot->mouseUp(e);
    const auto after = plugin.EditableDocument();
    Check(before != after && b.history->CanUndo(), "Modal drag not recorded");
    reject = true;
    editor.Undo();
    Check(b.history->CanUndo() && !b.history->CanRedo() &&
              plugin.EditableDocument() == after,
          "Failed undo advanced history");
    reject = false;
#ifdef __APPLE__
    const int modifier = visage::kModifierCmd;
#else
    const int modifier = visage::kModifierRegCtrl;
#endif
    Check(!editor.keyPress({visage::KeyCode::Z, 0, true}), "Plain Z hijacked");
    Check(editor.keyPress({visage::KeyCode::Z, modifier, true}),
          "Undo shortcut missing");
    Check(plugin.EditableDocument() == before && !b.history->CanUndo(),
          "A complete painting gesture must undo in one step");
    editor.keyPress(
        {visage::KeyCode::Z, modifier | visage::kModifierShift, true});
    Check(plugin.EditableDocument() == after,
          "Redo did not restore exact document");
  }
  {
    auto ownedEditor = std::make_unique<ui::Workbench>(b);
    auto &reopened = *ownedEditor;
    Check(b.history->CanUndo(), "Reopening editor lost history");
    reopened.Undo();
    Check(!b.history->CanUndo() && b.history->CanRedo(),
          "Reopened undo failed");
  }
}
