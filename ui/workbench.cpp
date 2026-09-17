#include "workbench.hpp"
#include <algorithm>
#include <exception>

namespace drumfoundry::ui {
namespace {
constexpr const char *names[]{"Kick",  "Snare", "Hi-hat",
                              "Crash", "Ride",  "Gong"};
constexpr const char *factoryIds[]{"factory.kick",  "factory.snare",
                                   "factory.hihat", "factory.crash",
                                   "factory.ride",  "factory.gong"};
} // namespace
Workbench::Workbench(Bridge bridge) : bridge_(std::move(bridge)) {
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &preset_, &undo_, &redo_, &settings_, &limiter_, &master_,
           &excitation_, &resonance_, &excitationTab_,
           &resonanceTab_, &columnSplit_, &footer_})
    addChild(frame);
  SetupHistory();
  SetupPanels();
  SetupLayout();
  SetupRouting();
  SetupFiles();
  SetupPerformance();
  footer_.setOnTop(
      true); // Errors remain readable while a modal dialog is open.
  addChild(&help_, false);
  help_.Bind(*this);
  NativeFonts(*this);
  ControlErrors(*this, [this](const auto &text) { Error(text); });
  timer_.onTimerCallback() = [this] { Poll(); };
  timer_.startTimer(33);
  Poll();
}
Workbench::~Workbench() {
  timer_.stopTimer();
  try {
    FinishLiveEdit();
  } catch (...) { /* Destructor must not throw. */
  }
}
void Workbench::Error(const std::string &message) {
  error_ = message;
  redraw();
}
void Workbench::SelectPreset() {
  try {
    visage::PopupMenu menu;
    menu.addOption(-1, "Factory presets").enable(false);
    for (int i = 0; i < 6; ++i)
      menu.addOption(i, names[i])
          .select(document_.JsonValue().value("id", "") == factoryIds[i]);
    menu.addBreak();
    menu.addOption(-1, "User presets").enable(false);
    std::vector<std::filesystem::path> paths;
    try {
      const auto root = UserPresetDirectory();
      paths = UserPresets(root);
      AddPresetFolders(menu, root, paths);
    } catch (const std::exception &e) {
      Error(e.what()); // Factory presets and import/save remain accessible.
    }
    if (paths.empty())
      menu.addOption(-1, "No user presets — choose a folder in Settings")
          .enable(false);
    menu.addBreak();
    menu.addOption(200, "Save preset as…");
    menu.addOption(201, "Import preset…");
    menu.addOption(202, "Export preset…");
    menu.onSelection() = [this, paths](int index) {
      try {
        if (index >= 1000 && unsigned(index - 1000) < paths.size()) {
          LoadPreset(editing::ReadFit(paths[index - 1000]));
        } else if (index == 200) {
          presetShade_.setVisible(true);
          presetSave_.Open(CaptureDocument());
        } else if (index == 201 || index == 202) {
          OpenFitFile(index == 202, CaptureDocument());
        } else if (index >= 0 && index < 6)
          LoadFactoryPreset(unsigned(index));
      } catch (const std::exception &e) {
        Error(e.what());
      }
    };
    menu.show(&preset_);
  } catch (const std::exception &e) {
    Error(e.what());
  }
}
void Workbench::RefreshDocument() {
  gestureKeys_.clear();
  gestureDocument_ = nullptr;
  liveEditBefore_ = nullptr;
  holdDecay_.Cancel();
  help_.Hide();
  metaShade_.setVisible(false);
  document_.Load(bridge_.document());
  displayedDocument_ = document_.JsonValue();
  routingShade_.setVisible(false);
  routing_.Load(document_);
  routes_.Load(document_);
  const auto &event = document_.JsonValue().at("controls").at("event");
  hardness_.SetDefault(event.at("hardness"));
  spread_.SetDefault(event.at("contactSpread"));
  location_.SetDefault(event.at("location"));
  mute_.SetDefault(event.at("constraint"));
  documentPreset_ = int(bridge_.value(100));
  documentRevision_ = bridge_.revision ? bridge_.revision() : 0;
  reloadDocument_ = false;
  excitation_.Load(document_, false);
  resonance_.Load(document_, true);
  playing_.Load(document_, false);
  modal_.Load(document_);
  analysis_.SetDocument(document_.JsonValue());
  ApplyTextSize();
  resized();
  NativeFonts(*this);
  ControlErrors(*this, [this](const auto &text) { Error(text); });
  help_.Bind(*this);
  preview_.Reset(document_.JsonValue());
}
void Workbench::ApplyDocument() {
  try {
    // Preserve the current performance controls rather than restoring the
    // gesture that happened to be active when the panel was populated.
    const auto current = bridge_.document();
    auto next = MergedEdit(current);
    const auto beforePreset = unsigned(bridge_.value(100));
    next["controls"]["event"] = current.at("controls").at("event");
    next["reference"] = current.at("reference");
    next["controls"]["analysis"] = current.at("controls").at("analysis");
    bridge_.applyDocument(next);
    const auto before = liveEditBefore_.is_null()
                            ? current
                            : LiveEditBaseline(bridge_.document());
    RecordDocument(before, bridge_.document(),
                   liveEditBefore_.is_null() ? beforePreset : liveEditPreset_,
                   applyingHold_);
    liveEditBefore_ = nullptr;
    gestureKeys_.clear();
    gestureDocument_ = nullptr;
    if (next.at("instrument").at("nodes").size() !=
        current.at("instrument").at("nodes").size()) {
      reloadDocument_ = true; // Added/removed owners require new control rows.
      return;
    }
    // Merge incoming automation into the editable model too; otherwise the
    // next gesture could mistake stale widget values for intentional edits.
    std::vector<std::pair<std::string, double>> accepted;
    for (const auto &node : next.at("instrument").at("nodes"))
      for (const auto &[key, value] : node.at("parameters").items())
        if (document_.Value(key) != value.get<double>())
          accepted.emplace_back(key, value.get<double>());
    document_.SetMany(accepted);
    routing_.Load(document_);
    excitation_.SyncValues(document_);
    resonance_.SyncValues(document_);
    playing_.SyncValues(document_);
    displayedDocument_ = next;
    analysis_.UpdateModel(next);
    preview_.Reset(next);
    modal_.Refresh();
    documentRevision_ = bridge_.revision ? bridge_.revision() : 0;
    if (!applyingHold_)
      holdDecay_.Edited(before, next, analysis_.RenderRate());
  } catch (const std::exception &e) {
    Error(e.what());
    reloadDocument_ = true;
  }
}
void Workbench::PreviewLiveDocument() {
  if (!bridge_.liveDocument)
    return;
  try {
    const auto current = bridge_.document();
    auto next = MergedEdit(current);
    next["controls"]["event"] = current.at("controls").at("event");
    next["reference"] = current.at("reference");
    next["controls"]["analysis"] = current.at("controls").at("analysis");
    if (next == current)
      return;
    if (!bridge_.liveDocument(next))
      return;
    if (liveEditBefore_.is_null()) {
      liveEditBefore_ = current;
      liveEditPreset_ = unsigned(bridge_.value(100));
    }
    // Do not rebuild widgets in response to our own live publication.
    documentRevision_ = bridge_.revision ? bridge_.revision() : 0;
  } catch (const std::exception &e) {
    Error(e.what());
  }
}
void Workbench::FinishLiveEdit() {
  if (liveEditBefore_.is_null())
    return;
  if (bridge_.finishLive)
    bridge_.finishLive();
  const auto after = bridge_.document();
  RecordDocument(LiveEditBaseline(after), after, liveEditPreset_);
  liveEditBefore_ = nullptr;
  gestureKeys_.clear();
  gestureDocument_ = nullptr;
}
} // namespace drumfoundry::ui
