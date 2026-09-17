#include "controls.hpp"
#include <algorithm>
#include <cmath>

namespace drumfoundry::ui {
namespace {
const auto &VelocityColour = colours::AxisVelocity;
const auto &PositionColour = colours::AxisPosition;
} // namespace
visage::Bounds StrikePad::PlayingBounds() const {
  return {18, 37, std::max(1.f, width() - 36), std::max(1.f, height() - 73)};
}
void StrikePad::draw(visage::Canvas &c) {
  c.setColor(colours::Strike);
  c.roundedRectangle(0, 0, width(), height(), 9);
  c.setColor(colours::Border);
  c.roundedRectangleBorder(1, 1, width() - 2, height() - 2, 8, 1);
  // A subdued grid suggests a playable surface rather than a text panel.
  const auto area = PlayingBounds();
  c.setColor(c.color(colours::Grid).withMultipliedAlpha(.5f));
  for (float fraction : {.25f, .5f, .75f}) {
    const float x = area.x() + area.width() * fraction;
    const float y = area.y() + area.height() * fraction;
    c.segment(x, area.y(), x, area.bottom(), 1, false);
    c.segment(area.x(), y, area.right(), y, 1, false);
  }
  Marker(c);
  Label(c, "STRIKE", 14, 7, 110, 24, PositionColour);
  const std::string hint =
      struck_ ? "Last: " +
                    std::to_string(int(std::round(
                        std::clamp(1 - lastY_, .01f, 1.f) * 100))) +
                    "%"
              : "Click to play";
  c.setColor(colours::Muted);
  c.text(hint, FrameFont(*this), visage::Font::kRight, width() - 140, 7, 126,
         24);
  Axes(c);
}
void StrikePad::Axes(visage::Canvas &c) {
  const auto area = PlayingBounds();
  const float bottom = area.bottom(), right = area.right(), left = area.x();
  c.setColor(VelocityColour);
  c.segment(left, bottom, left, area.y() + 6, 2, false);
  c.triangle(left, area.y(), left - 5, area.y() + 9, left + 5, area.y() + 9);
  Label(c, "Strong", 28, 35, 90, 22, VelocityColour);
  Label(c, "Velocity", 28, 58, 100, 22, VelocityColour);
  Label(c, "Light", 28, bottom - 24, 75, 22, VelocityColour);

  c.setColor(PositionColour);
  c.segment(left, bottom, right - 6, bottom, 2, false);
  c.triangle(right, bottom, right - 9, bottom - 5, right - 9, bottom + 5);
  const char *axis = kick_ ? "Beater hardness" : "Strike location";
  c.text(axis, FrameFont(*this), visage::Font::kRight, 100, bottom - 25,
         width() - 118, 22);
  Label(c,
        kick_       ? "Soft"
        : membrane_ ? "Centre"
                    : "Bell",
        18, bottom + 7, 90, 22, PositionColour);
  c.setColor(PositionColour);
  c.text(kick_ ? "Hard" : "Edge", FrameFont(*this), visage::Font::kRight,
         width() - 98, bottom + 7, 80, 22);
  if (!kick_ && !membrane_)
    c.text("Bow", FrameFont(*this), visage::Font::kCenter, width() / 2 - 35,
           bottom + 7, 70, 22);
}
void StrikePad::Marker(visage::Canvas &c) {
  if (!struck_)
    return;
  const auto area = PlayingBounds();
  const float x = area.x() + lastX_ * area.width();
  const float y = area.y() + lastY_ * area.height();
  c.setColor(c.color(colours::Accent).withMultipliedAlpha(.16f));
  c.circle(x - 16, y - 16, 32);
  c.setColor(c.color(colours::Accent).withMultipliedAlpha(.5f));
  c.segment(area.x(), y, area.right(), y, 1, false);
  c.segment(x, area.y(), x, area.bottom(), 1, false);
  c.setColor(colours::Text);
  c.circle(x - 4, y - 4, 8);
}
void StrikePad::ShowStrike(float velocity, float position) {
  lastX_ = std::clamp(position, 0.f, 1.f);
  lastY_ = 1.f - std::clamp(velocity, 0.f, 1.f);
  struck_ = true;
  redraw();
}
void StrikePad::mouseDown(const visage::MouseEvent &e) {
  if (!e.isLeftButton() || height() <= 0 || width() <= 0)
    return;
  const auto area = PlayingBounds();
  lastX_ = std::clamp((e.position.x - area.x()) / area.width(), 0.f, 1.f);
  lastY_ = std::clamp((e.position.y - area.y()) / area.height(), 0.f, 1.f);
  struck_ = true;
  redraw();
  // Margins still play, clamped to the corresponding labelled endpoint.
  if (strike)
    strike(std::clamp(1.f - lastY_, .01f, 1.f), lastX_);
}
} // namespace drumfoundry::ui
