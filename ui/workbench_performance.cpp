#include "workbench.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
namespace drumfoundry::ui {
bool Workbench::EnsureAudio() {
  if (!bridge_.audioRunning || bridge_.audioRunning())
    return true;
  Error("Audio stopped. Select your device and press Apply & start.");
  if (bridge_.settings)
    bridge_.settings();
  return false;
}
void Workbench::SetupPerformance() {
  for (unsigned i = 0; i < implements_.size(); ++i) {
    right_.addScrolledChild(&implements_[i]);
    implements_[i].onToggle() = [this, i](auto *, bool) {
      EditPerformance(102, i * .5);
      CommitPerformance();
    };
  }
  preset_.onToggle() = [this](auto *, bool) { SelectPreset(); };
  limiter_.onToggle() = [this](auto *, bool) {
    EditPerformance(106, bridge_.value(106) < .5);
    CommitPerformance();
  };
  settings_.onToggle() = [this](auto *, bool) { OpenSettings(); };
  master_.changed = [this](double v) { EditPerformance(105, v); };
  hardness_.changed = [this](double v) { EditPerformance(101, v); };
  spread_.changed = [this](double v) { EditPerformance(109, v); };
  velocity_.changed = [this](double v) { EditPerformance(0, v); };
  location_.changed = [this](double v) { EditPerformance(103, v); };
  mute_.changed = [this](double v) { EditPerformance(104, v); };
  for (auto *slider :
       {&master_, &hardness_, &spread_, &velocity_, &location_, &mute_})
    slider->committed = [this] { CommitPerformance(); };
  strike_.strike = [this](float v, float x) {
    try {
      if (!EnsureAudio())
        return;
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
