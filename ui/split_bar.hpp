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
    redraw();
    if (started)
      started();
  }
  void mouseDrag(const visage::MouseEvent &event) override {
    if (dragging_ && dragged)
      dragged(Coordinate(event) - origin_);
  }
  void mouseUp(const visage::MouseEvent &) override {
    dragging_ = false;
    redraw();
  }
  void mouseEnter(const visage::MouseEvent &) override {
    hover_ = true;
    setCursorStyle(vertical ? visage::MouseCursor::HorizontalResize
                            : visage::MouseCursor::VerticalResize);
    redraw();
  }
  void mouseExit(const visage::MouseEvent &) override {
    hover_ = false;
    setCursorStyle(visage::MouseCursor::Arrow);
    redraw();
  }
  void draw(visage::Canvas &c) override {
    // Keep the generous input bounds invisible; only paint a fine divider.
    const float thickness = hover_ || dragging_ ? 3.f : 1.f;
    c.setColor(colours::Track);
    if (vertical)
      c.fill(width() * .5f - .5f, 0, 1, height());
    else
      c.fill(0, height() * .5f - .5f, width(), 1);
    c.setColor(hover_ || dragging_ ? colours::Accent : colours::Muted);
    if (vertical)
      c.roundedRectangle((width() - thickness) * .5f, height() * .5f - 20,
                         thickness, 40, thickness * .5f);
    else
      c.roundedRectangle(width() * .5f - 20, (height() - thickness) * .5f, 40,
                         thickness, thickness * .5f);
  }

private:
  float Coordinate(const visage::MouseEvent &event) const {
    return vertical ? event.windowPosition().x : event.windowPosition().y;
  }
  float origin_{};
  bool dragging_{};
  bool hover_{};
};
} // namespace drumfoundry::ui
