#pragma once
#include "controls.hpp"
#include <utility>

namespace drumfoundry::ui {
// Editor-only audition state. Does not change presets, MIDI, or voice parameters.
class StrikeFreeze : public visage::Frame {
public:
  StrikeFreeze();
  void Open(float velocity, float position, bool kick);
  void SetKick(bool kick);
  bool Enabled() const { return enabled_; }
  std::pair<float, float> Apply(float velocity, float position) const;
  void resized() override;
  void draw(visage::Canvas &) override;
  bool keyPress(const visage::KeyEvent &) override;
  std::function<void()> close, changed;

private:
  bool enabled_{}, kick_{};
  float velocityValue_{.8f}, positionValue_{.5f};
  Slider velocity_{"Velocity", 0, 1, .8}, position_{"Strike location", 0, 1, .5};
  HelpButton enabledButton_{"Freeze OFF"};
  visage::UiButton close_{"Done"};
};
} // namespace drumfoundry::ui
