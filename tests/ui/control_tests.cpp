#include "ui/controls.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

void Require(bool condition) {
  if (!condition)
    throw std::runtime_error("UI control regression");
}
int main() {
  using namespace drumfoundry::ui;
  Slider slider("Test", -60, 0, -12);
  slider.setBounds(0, 0, 212, 44);
  slider.Set(100);
  Require(slider.Value() == 0);
  slider.Set(std::numeric_limits<double>::quiet_NaN());
  Require(slider.Value() == 0);
  double notified = 999;
  slider.changed = [&](double value) { notified = value; };
  visage::MouseEvent event;
  event.button_id = visage::kMouseButtonLeft;
  event.repeat_click_count = 2;
  slider.mouseDown(event);
  Require(notified == -12);
  event.repeat_click_count = 1;
  event.position.x = 106;
  slider.mouseDown(event);
  Require(slider.Value() == -30);
  event.modifiers = visage::kModifierShift;
  slider.mouseDown(event);
  event.position.x += 100;
  slider.mouseDrag(event);
  Require(std::abs(slider.Value() + 27) < 1e-9);
  StrikePad pad;
  pad.setBounds(0, 0, 200, 100);
  float velocity = 0, location = 0;
  pad.strike = [&](float v, float x) {
    velocity = v;
    location = x;
  };
  event.position = {50, 25};
  pad.mouseDown(event);
  Require(velocity == .75f && location == .25f);
  return 0;
}
