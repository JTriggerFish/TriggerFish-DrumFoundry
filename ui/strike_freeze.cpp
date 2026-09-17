#include "strike_freeze.hpp"
#include <algorithm>

namespace drumfoundry::ui {
StrikeFreeze::StrikeFreeze() {
  setName("strike-freeze-popup");
  setAcceptsKeystrokes(true);
  addChild(&velocity_);
  addChild(&position_);
  addChild(&enabledButton_);
  addChild(&close_);
  enabledButton_.setName("freeze-enabled");
  velocity_.setName("frozen-velocity");
  position_.setName("frozen-position");
  enabledButton_.help = "Use these fixed values wherever you click the strike pad. MIDI is unchanged.";
  enabledButton_.onToggle() = [this](auto *, bool) {
    enabled_ = !enabled_;
    enabledButton_.setText(enabled_ ? "Freeze ON" : "Freeze OFF");
    enabledButton_.setActionButton(enabled_);
    if (changed) changed();
  };
  velocity_.changed = [this](double v) { velocityValue_ = float(v); };
  position_.changed = [this](double v) { positionValue_ = float(v); };
  close_.onToggle() = [this](auto *, bool) { if (close) close(); };
}
void StrikeFreeze::SetKick(bool kick) {
  if (kick_ != kick) {
    // Location and beater hardness are different coordinates: don't carry a
    // locked position across that topology change without explicit selection.
    enabled_ = false;
    enabledButton_.setText("Freeze OFF");
    enabledButton_.setActionButton(false);
  }
  kick_ = kick;
  position_.SetLabel(kick ? "Beater hardness" : "Strike location");
}
void StrikeFreeze::Open(float velocity, float position, bool kick) {
  SetKick(kick);
  if (!enabled_) {
    velocityValue_ = std::clamp(velocity, 0.f, 1.f);
    positionValue_ = std::clamp(position, 0.f, 1.f);
  }
  velocity_.Set(velocityValue_);
  position_.Set(positionValue_);
  velocity_.SetDefault(velocityValue_);
  position_.SetDefault(positionValue_);
  requestKeyboardFocus();
}
std::pair<float, float> StrikeFreeze::Apply(float velocity, float position) const {
  return enabled_ ? std::pair{velocityValue_, positionValue_}
                  : std::pair{velocity, position};
}
void StrikeFreeze::resized() {
  velocity_.setBounds(16, 44, width() - 32, 56);
  position_.setBounds(16, 108, width() - 32, 56);
  enabledButton_.setBounds(16, 180, 150, 30);
  close_.setBounds(width() - 108, 180, 92, 30);
}
void StrikeFreeze::draw(visage::Canvas &c) {
  c.setColor(colours::Border);
  c.roundedRectangle(0, 0, width(), height(), 8);
  c.setColor(colours::Panel);
  c.roundedRectangle(1, 1, width() - 2, height() - 2, 8);
  Label(c, "FREEZE STRIKE", 16, 8, width() - 32, 26, colours::Heading);
}
bool StrikeFreeze::keyPress(const visage::KeyEvent &event) {
  if (event.keyCode() != visage::KeyCode::Escape) return false;
  if (close) close();
  return true;
}
} // namespace drumfoundry::ui
