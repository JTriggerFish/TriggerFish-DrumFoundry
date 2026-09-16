#include "workbench.hpp"
#include <exception>

namespace drumfoundry::ui {
namespace {
editing::Json Design(editing::Json document) {
  document["controls"].erase("analysis");
  return document;
}
bool EditingText(const visage::Frame &frame) {
  if (!frame.isVisible())
    return false;
  if (frame.hasKeyboardFocus() &&
      dynamic_cast<const visage::TextEditor *>(&frame))
    return true;
  for (auto *child : frame.children())
    if (EditingText(*child))
      return true;
  return false;
}
} // namespace
void Workbench::SetupHistory() {
  if (!bridge_.history)
    bridge_.history = std::make_shared<EditHistory>();
  setAcceptsKeystrokes(true);
  undo_.help =
      "Undo the last edit. One step per drag; playback and zoom are not edits.";
  redo_.help = "Redo an undone edit. A new edit clears the redo branch.";
#ifdef __APPLE__
  undo_.help += " Shortcut: Cmd+Z.";
  redo_.help += " Shortcut: Cmd+Shift+Z.";
#else
  undo_.help += " Shortcut: Ctrl+Z.";
  redo_.help += " Shortcut: Ctrl+Y or Ctrl+Shift+Z.";
#endif
  undo_.onToggle() = [this](auto *, bool) { Undo(); };
  redo_.onToggle() = [this](auto *, bool) { Redo(); };
  UpdateHistoryButtons();
}
void Workbench::UpdateHistoryButtons() {
  undo_.setActive(bridge_.history->CanUndo());
  redo_.setActive(bridge_.history->CanRedo());
  undo_.redraw();
  redo_.redraw();
}
editing::Json Workbench::HistoryState() const {
  editing::Json result{{"document", Design(bridge_.document())},
                       {"preset", unsigned(bridge_.value(100))}};
  for (unsigned id : {101, 102, 103, 104, 105, 106, 109})
    result["parameters"][std::to_string(id)] = bridge_.value(id);
  result["parameters"]["velocity"] = bridge_.velocity ? bridge_.velocity() : .8;
  return result;
}
void Workbench::RecordDocument(const editing::Json &before,
                               const editing::Json &after,
                               unsigned beforePreset, bool merge) {
  bridge_.history->Record(
      {{"document", Design(before)}, {"preset", beforePreset}},
      {{"document", Design(after)}, {"preset", unsigned(bridge_.value(100))}},
      merge);
  UpdateHistoryButtons();
}
void Workbench::LoadPreset(const editing::Json &next) {
  CommitPerformance();
  const auto before = bridge_.document();
  const auto beforePreset = unsigned(bridge_.value(100));
  bridge_.applyDocument(next);
  RecordDocument(before, bridge_.document(), beforePreset);
  reloadDocument_ = true;
}
void Workbench::Undo() { RestoreHistory(false); }
void Workbench::Redo() { RestoreHistory(true); }
void Workbench::LoadFactoryPreset(unsigned index) {
  if (!bridge_.selectFactory)
    throw std::runtime_error("The host bridge cannot load factory presets");
  CommitPerformance();
  const auto before = bridge_.document();
  const auto preset = unsigned(bridge_.value(100));
  bridge_.selectFactory(index);
  RecordDocument(before, bridge_.document(), preset);
  reloadDocument_ = true;
}
void Workbench::RestoreHistory(bool redo) {
  try {
    CommitPerformance();
    auto &history = *bridge_.history;
    if (redo ? !history.CanRedo() : !history.CanUndo())
      return;
    const auto current = HistoryState();
    const auto target = history.Target(current, redo);
    holdDecay_.Cancel();
    const bool presetChanged = target.at("preset") != current.at("preset");
    if (target.at("document") != current.at("document") || presetChanged) {
      auto next = target.at("document");
      next["controls"]["analysis"] =
          bridge_.document().at("controls").at("analysis");
      auto referenceOnly = current.at("document");
      referenceOnly["reference"] = next.at("reference");
      if (!presetChanged && referenceOnly == target.at("document") &&
          bridge_.presentation)
        bridge_.presentation(next.at("reference"),
                             next.at("controls").at("analysis"));
      else if (bridge_.restoreDocument)
        bridge_.restoreDocument(next, target.at("preset"));
      else if (presetChanged)
        throw std::runtime_error(
            "The host bridge cannot restore the factory selector");
      else
        bridge_.applyDocument(next);
      reloadDocument_ = true;
    }
    // Re-send every edited parameter even if readback still shows that value:
    // an earlier queued undo may not have reached the audio thread yet.
    for (const auto &operation : history.Patch(redo)) {
      const auto path = operation.at("path").get<std::string>();
      if (path.rfind("/parameters/", 0) != 0)
        continue;
      const auto key = path.substr(12);
      const double value = target.at("parameters").at(key);
      if (key == "velocity") {
        if (bridge_.setVelocity)
          bridge_.setVelocity(value);
      } else
        bridge_.change(unsigned(std::stoul(key)), value);
    }
    history.Accept(redo);
    UpdateHistoryButtons();
  } catch (const std::exception &e) {
    Error(std::string("Could not restore edit: ") + e.what());
  }
}
bool Workbench::keyPress(const visage::KeyEvent &e) {
#ifdef __APPLE__
  const bool modifier = e.isCmdDown() && !e.isCtrlDown();
#else
  const bool modifier = e.isCtrlDown() && !e.isCmdDown();
#endif
  if (!modifier || e.isAltDown())
    return false;
  const bool z = e.keyCode() == visage::KeyCode::Z;
  const bool y = e.keyCode() == visage::KeyCode::Y && !e.isShiftDown();
#ifdef __APPLE__
  if (!z)
#else
  if (!z && !y)
#endif
    return false;
  // TextEditor handles its own undo first. If its history is empty, don't
  // let the unhandled keystroke fall through and change the instrument.
  if (EditingText(*this))
    return true;
  if (y || e.isShiftDown())
    Redo();
  else
    Undo();
  return true;
}
} // namespace drumfoundry::ui
