#include "workbench.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
namespace drumfoundry::ui {
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
  spread_.changed = [this](double v) { Change(109, v); };
  velocity_.changed = [this](double v) {
    try {
      if (bridge_.setVelocity)
        bridge_.setVelocity(v);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  location_.changed = [this](double v) { Change(103, v); };
  mute_.changed = [this](double v) { Change(104, v); };
  strike_.strike = [this](float v, float x) {
    try {
      bridge_.strike(v, x);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  fixedStrike_.onToggle() = [this](auto *, bool) {
    const float velocity = bridge_.velocity ? float(bridge_.velocity()) : .8f;
    if (velocity > 0)
      strike_.strike(velocity,
                     float(bridge_.value(
                         document_.Recipe() == "drum.kick.v1" ? 101 : 103)));
  };
}
} // namespace drumfoundry::ui
