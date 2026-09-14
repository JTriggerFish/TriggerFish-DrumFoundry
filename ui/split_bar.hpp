#pragma once
#include "controls.hpp"
namespace drumfoundry::ui {
// Window-relative deltas avoid feedback/jumps as the divider itself moves.
class SplitBar : public visage::Frame {
public:
  bool vertical{}; // A vertical divider moves horizontally.
  std::function<void()> started;
  std::function<void(float)> dragged;
  void mouseDown(const visage::MouseEvent &event) override {
    if (!event.isLeftButton())
      return;
    dragging_ = true;
    origin_ = Coordinate(event);
    if (started)
      started();
  }
  void mouseDrag(const visage::MouseEvent &event) override {
    if (dragging_ && dragged)
      dragged(Coordinate(event) - origin_);
  }
  void mouseUp(const visage::MouseEvent &) override { dragging_ = false; }
  void draw(visage::Canvas &c) override {
    c.setColor(colours::Border);
    if (vertical)
      c.roundedRectangle(width() * .5f - 1, height() * .35f, 2,
                         height() * .3f, 1);
    else
      c.roundedRectangle(width() * .35f, height() * .5f - 1, width() * .3f, 2,
                         1);
  }

private:
  float Coordinate(const visage::MouseEvent &event) const {
    return vertical ? event.windowPosition().x : event.windowPosition().y;
  }
  float origin_{};
  bool dragging_{};
};
} // namespace drumfoundry::ui
