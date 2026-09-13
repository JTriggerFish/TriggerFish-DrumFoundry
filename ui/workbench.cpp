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
           &preset_, &settings_, &stop_, &limiter_, &master_, &hardness_,
           &implement_, &location_, &mute_, &strike_})
    addChild(frame);
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
  implement_.changed = [this](double v) { Change(102, v); };
  location_.changed = [this](double v) { Change(103, v); };
  mute_.changed = [this](double v) { Change(104, v); };
  strike_.strike = [this](float v, float x) {
    try {
      bridge_.strike(v, x);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
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
  menu.onSelection() = [this](int index) { Change(100, index); };
  menu.show(&preset_);
}
void Workbench::Poll() {
  try {
    if (bridge_.service)
      bridge_.service();
    preset_.setText(names[std::clamp(int(bridge_.value(100)), 0, 5)]);
    master_.Set(bridge_.value(105));
    hardness_.Set(bridge_.value(101));
    implement_.Set(bridge_.value(102));
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
} // namespace drumfoundry::ui
