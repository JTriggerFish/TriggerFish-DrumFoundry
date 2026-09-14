#include "workbench.hpp"
#include <algorithm>
#include <exception>

namespace drumfoundry::ui {
namespace {
constexpr const char *names[]{"Kick",  "Snare", "Hi-hat",
                              "Crash", "Ride",  "Gong"};
}
Workbench::Workbench(Bridge bridge) : bridge_(std::move(bridge)) {
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &preset_, &settings_, &limiter_, &master_, &location_, &mute_,
           &excitation_, &resonance_, &excitationTab_, &resonanceTab_,
           &columnSplit_})
    addChild(frame);
  SetupPanels();
  SetupLayout();
  SetupRouting();
  SetupFiles();
  SetupPerformance();
  addChild(&help_, false);
  help_.Bind(*this);
  NativeFonts(*this);
  ControlErrors(*this, [this](const auto &text) { Error(text); });
  timer_.onTimerCallback() = [this] { Poll(); };
  timer_.startTimer(33);
  Poll();
}
Workbench::~Workbench() { timer_.stopTimer(); }
void Workbench::Error(const std::string &message) {
  error_ = message;
  redraw();
}
void Workbench::Change(unsigned id, double value) {
  try {
    bridge_.change(id, value);
  } catch (const std::exception &e) {
    Error(e.what());
  }
}
void Workbench::SelectPreset() {
  visage::PopupMenu menu;
  for (int i = 0; i < 6; ++i)
    menu.addOption(i, names[i]).select(bridge_.value(100) == i);
  if (bridge_.selectCalibration) {
    visage::PopupMenu calibrations("Calibrations (with reference)");
    for (int i = 0; i < 6; ++i)
      calibrations.addOption(100 + i, names[i]);
    menu.addSubMenu(std::move(calibrations));
  }
  menu.onSelection() = [this](int index) {
    try {
      if (index >= 100 && bridge_.selectCalibration)
        bridge_.selectCalibration(unsigned(index - 100));
      else if (bridge_.selectFactory)
        bridge_.selectFactory(unsigned(index));
      else
        Change(100, index);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  menu.show(&preset_);
}
void Workbench::RefreshDocument() {
  holdDecay_.Cancel();
  help_.Hide();
  metaShade_.setVisible(false);
  document_.Load(bridge_.document());
  routingShade_.setVisible(false);
  routing_.Load(document_);
  routes_.Load(document_);
  const auto &event = document_.JsonValue().at("controls").at("event");
  velocity_.SetDefault(event.at("strength"));
  hardness_.SetDefault(event.at("hardness"));
  spread_.SetDefault(event.at("contactSpread"));
  location_.SetDefault(event.at("location"));
  mute_.SetDefault(event.at("constraint"));
  documentPreset_ = int(bridge_.value(100));
  documentRevision_ = bridge_.revision ? bridge_.revision() : 0;
  reloadDocument_ = false;
  excitation_.Load(document_, false);
  resonance_.Load(document_, true);
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
    auto next = document_.JsonValue();
    // Preserve the current performance controls rather than restoring the
    // gesture that happened to be active when the panel was populated.
    const auto current = bridge_.document();
    next["controls"]["event"] = current.at("controls").at("event");
    next["reference"] = current.at("reference");
    next["controls"]["analysis"] = current.at("controls").at("analysis");
    bridge_.applyDocument(next);
    analysis_.UpdateModel(next);
    preview_.Reset(next);
    modal_.Refresh();
    documentRevision_ = bridge_.revision ? bridge_.revision() : 0;
    if (!applyingHold_)
      holdDecay_.Edited(current, next, analysis_.RenderRate());
  } catch (const std::exception &e) {
    Error(e.what());
    reloadDocument_ = true;
  }
}
} // namespace drumfoundry::ui
