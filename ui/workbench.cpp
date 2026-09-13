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
           &preset_, &settings_, &stop_, &limiter_, &master_, &location_,
           &mute_, &excitation_, &resonance_, &referencePlay_, &modelPlay_})
    addChild(frame);
  SetupPanels();
  SetupFiles();
  SetupPerformance();
  NativeFonts(*this);
  timer_.onTimerCallback() = [this] { Poll(); };
  timer_.startTimer(33);
  Poll();
}
void Workbench::SetupPanels() {
  for (auto *panel : {&excitation_, &resonance_}) {
    panel->committed = [this] { ApplyDocument(); };
    panel->error = [this](const auto &text) { Error(text); };
    panel->meta = [this](bool size) {
      meta_.Open(document_, size);
      metaShade_.setVisible(true);
    };
  }
  addChild(&right_);
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &analysis_, &modal_, &strike_, &hardness_})
    right_.addScrolledChild(frame);
  analysis_.chooseReference = [this] { OpenReferenceFile(); };
  analysis_.error = [this](const auto &message) { Error(message); };
  analysis_.play = bridge_.play;
  analysis_.presentation = bridge_.presentation;
  referencePlay_.onToggle() = [this](auto *, bool) { analysis_.Play(true); };
  modelPlay_.onToggle() = [this](auto *, bool) { analysis_.Play(false); };
  modal_.committed = [this] { ApplyDocument(); };
  modal_.error = [this](const auto &text) { Error(text); };
  addChild(&metaShade_, false);
  metaShade_.setOnTop(true);
  metaShade_.addChild(&meta_);
  metaShade_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(0x4005090f);
    c.fill(0, 0, width(), height());
  };
  meta_.onVisibilityChange() = [this] {
    if (!meta_.isVisible())
      metaShade_.setVisible(false);
  };
  meta_.changed = [this] {
    excitation_.Load(document_, false);
    resonance_.Load(document_, true);
    modal_.Refresh();
  };
  meta_.committed = [this] { ApplyDocument(); };
  meta_.error = [this](const auto &text) { Error(text); };
}
void Workbench::SetupFiles() {
  addChild(&history_);
  history_.capture = [this] { return CaptureDocument(); };
  history_.restore = [this](const auto &document) {
    bridge_.applyDocument(document);
    reloadDocument_ = true;
  };
  history_.file = [this](bool save, const auto &document) {
    OpenFitFile(save, document);
  };
  history_.error = files_.error = [this](const auto &text) { Error(text); };
  addChild(&fileShade_, false);
  fileShade_.setOnTop(true);
  fileShade_.addChild(&files_);
  fileShade_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(0xa005090f);
    c.fill(0, 0, width(), height());
  };
  files_.onVisibilityChange() = [this] {
    if (!files_.isVisible())
      fileShade_.setVisible(false);
  };
}
void Workbench::SetupPerformance() {
  for (unsigned i = 0; i < implements_.size(); ++i) {
    right_.addScrolledChild(&implements_[i]);
    implements_[i].onToggle() = [this, i](auto *, bool) {
      Change(102, i * .5);
    };
  }
  preset_.onToggle() = [this](auto *, bool) { SelectPreset(); };
  limiter_.onToggle() = [this](auto *, bool) {
    Change(106, bridge_.value(106) < .5);
  };
  stop_.onToggle() = [this](auto *, bool) {
    try {
      bridge_.stop();
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  settings_.setVisible(bool(bridge_.settings));
  settings_.onToggle() = [this](auto *, bool) {
    if (bridge_.settings)
      bridge_.settings();
  };
  master_.changed = [this](double v) { Change(105, v); };
  hardness_.changed = [this](double v) { Change(101, v); };
  location_.changed = [this](double v) { Change(103, v); };
  mute_.changed = [this](double v) { Change(104, v); };
  strike_.strike = [this](float v, float x) {
    try {
      bridge_.strike(v, x);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
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
  menu.onSelection() = [this](int index) { Change(100, index); };
  menu.show(&preset_);
}
void Workbench::Poll() {
  try {
    if (bridge_.service)
      bridge_.service();
    if (bridge_.document &&
        (reloadDocument_ || documentPreset_ != int(bridge_.value(100)) ||
         (bridge_.revision && documentRevision_ != bridge_.revision())))
      RefreshDocument();
    if (bridge_.sampleRate)
      analysis_.SetAuditionRate(bridge_.sampleRate());
    analysis_.Poll();
    // Host automation and UI performance changes use the same analysis path.
    // Briefly debounce drags; don't cancel a render on every timer tick.
    if (bridge_.document) {
      auto latest = bridge_.document();
      const auto &event = latest.at("controls").at("event");
      if (event != renderedEvent_) {
        renderedEvent_ = event;
        eventDebounce_ = 4;
      } else if (eventDebounce_ && --eventDebounce_ == 0) {
        analysis_.UpdateModel(latest);
      }
    }
    preset_.setText(names[std::clamp(int(bridge_.value(100)), 0, 5)]);
    master_.Set(bridge_.value(105));
    hardness_.Set(bridge_.value(101));
    const double implement = bridge_.value(102);
    for (unsigned i = 0; i < implements_.size(); ++i)
      implements_[i].setActionButton(std::abs(implement - i * .5) < .01);
    hardness_.SetLabel(implement < .25   ? "Bristle stiffness"
                       : implement < .75 ? "Mallet firmness"
                                         : "Tip hardness");
    const bool kick = bridge_.value(100) == 0;
    strike_.SetKick(kick);
    strike_.SetMembrane(bridge_.value(100) == 1);
    location_.setVisible(!kick);
    mute_.setVisible(bridge_.value(100) >= 2);
    location_.Set(bridge_.value(103));
    mute_.Set(bridge_.value(104));
    limiter_.setText(bridge_.value(106) >= .5 ? "Limiter ON" : "UNPROTECTED");
    reduction_ = bridge_.value(107);
    latency_ = bridge_.value(108);
    if (bridge_.status)
      status_ = bridge_.status();
    redraw();
  } catch (const std::exception &e) {
    Error(e.what());
  }
}
void Workbench::RefreshDocument() {
  metaShade_.setVisible(false);
  document_.Load(bridge_.document());
  documentPreset_ = int(bridge_.value(100));
  documentRevision_ = bridge_.revision ? bridge_.revision() : 0;
  reloadDocument_ = false;
  excitation_.Load(document_, false);
  resonance_.Load(document_, true);
  modal_.Load(document_);
  analysis_.SetDocument(document_.JsonValue());
  NativeFonts(*this);
  renderedEvent_ = document_.JsonValue().at("controls").at("event");
  eventDebounce_ = 0;
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
    renderedEvent_ = next.at("controls").at("event");
    eventDebounce_ = 0;
    modal_.Refresh();
    documentRevision_ = bridge_.revision ? bridge_.revision() : 0;
  } catch (const std::exception &e) {
    Error(e.what());
    reloadDocument_ = true;
  }
}
} // namespace drumfoundry::ui
