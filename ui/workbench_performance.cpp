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
  location_.setName("strike-location");
  mute_.setName("hand-mute");
  spread_.setName("contact-spread");
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
  location_.changed = [this](double v) { EditPerformance(103, v); };
  mute_.changed = [this](double v) { EditPerformance(104, v); };
  for (auto *slider :
       {&master_, &hardness_, &spread_, &location_, &mute_})
    slider->committed = [this] { CommitPerformance(); };
  strike_.strike = [this](float v, float x) {
    try {
      if (!EnsureAudio())
        return;
      const auto [velocity, position] = freeze_.Apply(v, x);
      bridge_.strike(velocity, position);
      strike_.ShowStrike(velocity, position);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  addChild(&freezeShade_, false);
  freezeShade_.setOnTop(true);
  freezeShade_.addChild(&freeze_);
  freezeShade_.onMouseDown() = [this](const auto &) { freezeShade_.setVisible(false); };
  freeze_.close = [this] { freezeShade_.setVisible(false); };
  freeze_.changed = [this] { freezeStrike_.setActionButton(freeze_.Enabled()); };
  freezeStrike_.setName("freeze-strike");
  freezeStrike_.help = "Set a fixed velocity and position for repeatable pad strikes. Highlighted while frozen; MIDI stays expressive.";
  freezeStrike_.onToggle() = [this](auto *, bool) {
    const bool kick = document_.Recipe() == "drum.kick.v1";
    freezeShade_.setVisible(true);
    freeze_.Open(bridge_.velocity ? float(bridge_.velocity()) : .8f,
                 float(bridge_.value(kick ? 101 : 103)), kick);
  };
}
} // namespace drumfoundry::ui
