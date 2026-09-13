#include "controls.hpp"
#include "embedded/fonts.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace drumfoundry::ui {
visage::Font Font(float size) {
  return visage::Font(size, visage::fonts::Lato_Regular_ttf);
}
void Label(visage::Canvas &c, const std::string &text, float x, float y,
           float w, float h, unsigned color) {
  c.setColor(color);
  c.text(text, Font(), visage::Font::kLeft, x, y, w, h);
}
Slider::Slider(std::string label, double low, double high, double initial,
               std::string unit)
    : label_(std::move(label)), unit_(std::move(unit)), low_(low), high_(high),
      initial_(initial), value_(initial) {}
void Slider::Set(double value) {
  if (!std::isfinite(value))
    return;
  value = std::clamp(value, low_, high_);
  if (value_ != value) {
    value_ = value;
    redraw();
  }
}
void Slider::Edit(double value) {
  Set(value);
  if (changed)
    changed(value_);
}
void Slider::draw(visage::Canvas &c) {
  Label(c, label_, 0, 0, width() * .65f, 22);
  char text[48];
  std::snprintf(text, sizeof(text), "%.2f%s", value_, unit_.c_str());
  Label(c, text, width() * .67f, 0, width() * .33f, 22);
  const float track = std::max(1.f, width() - 12.f);
  c.setColor(0xff303c49);
  c.roundedRectangle(6, 30, track, 4, 2);
  const float x = 6 + track * float((value_ - low_) / (high_ - low_));
  c.setColor(0xff9fcaff);
  c.roundedRectangle(6, 30, x - 6, 4, 2);
  c.circle(x - 5, 27, 10);
}
void Slider::mouseDown(const visage::MouseEvent &e) {
  if (!e.isLeftButton())
    return;
  if (e.repeatClickCount() == 2)
    Edit(initial_);
  else if (!e.isShiftDown())
    Edit(low_ +
         (high_ - low_) * (e.position.x - 6) / std::max(1.f, width() - 12.f));
  dragX_ = e.position.x;
  dragValue_ = value_;
}
void Slider::mouseDrag(const visage::MouseEvent &e) {
  const double fine = e.isShiftDown() ? .1 : 1.;
  Edit(dragValue_ + (e.position.x - dragX_) / std::max(1.f, width() - 12.f) *
                        (high_ - low_) * fine);
}
void StrikePad::draw(visage::Canvas &c) {
  c.setColor(0xff17202a);
  c.roundedRectangle(0, 0, width(), height(), 5);
  Label(c, "STRIKE  /  velocity", 12, 5, width() - 24, 22);
  Label(c, "Strong", 12, 30, 80, 20);
  Label(c, "Light", 12, height() - 26, 80, 20);
  Label(c, "Bell     —     Bow     —     Edge", width() * .35f, height() - 26,
        width() * .6f, 20);
}
void StrikePad::mouseDown(const visage::MouseEvent &e) {
  if (!e.isLeftButton() || height() <= 0 || width() <= 0)
    return;
  if (strike)
    strike(std::clamp(1.f - e.position.y / height(), .01f, 1.f),
           std::clamp(e.position.x / width(), 0.f, 1.f));
}
} // namespace drumfoundry::ui
