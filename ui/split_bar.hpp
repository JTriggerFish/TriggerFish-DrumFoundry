#pragma once
#include "controls.hpp"
namespace drumfoundry::ui {
// Window-relative deltas avoid feedback/jumps as the divider itself moves.
class SplitBar : public visage::Frame {
public:
  std::function<void()> started;
  std::function<void(float)> dragged;
  void mouseDown(const visage::MouseEvent &event) override {
    if (!event.isLeftButton())
      return;
    dragging_ = true;
    origin_ = event.windowPosition().y;
    if (started)
      started();
  }
  void mouseDrag(const visage::MouseEvent &event) override {
    if (dragging_ && dragged)
      dragged(event.windowPosition().y - origin_);
  }
  void mouseUp(const visage::MouseEvent &) override { dragging_ = false; }
  void draw(visage::Canvas &c) override {
    c.setColor(0xff536778);
    c.roundedRectangle(width() * .35f, height() * .5f - 1, width() * .3f, 2, 1);
  }

private:
  float origin_{};
  bool dragging_{};
};
} // namespace drumfoundry::ui
