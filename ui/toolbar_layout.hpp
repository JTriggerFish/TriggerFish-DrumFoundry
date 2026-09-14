#pragma once
#include "typography.hpp"
#include <algorithm>
#include <visage/ui.h>

namespace drumfoundry::ui {
// Compact controls wrap on narrow panels instead of stretching with the plot.
class ToolbarLayout {
public:
  ToolbarLayout(float width, float top, float rowHeight)
      : width_(std::max(1.f, width)), y_(top), rowHeight_(rowHeight) {}
  void Place(visage::Frame &frame, float wanted, float height = 28) {
    const float w = std::min(wanted * frame.paletteValue(TextScale), width_);
    if (x_ > 0 && x_ + w > width_) {
      x_ = 0;
      y_ += rowHeight_;
    }
    frame.setBounds(x_, y_, w, height);
    x_ += w + 8;
  }
  float Bottom() const { return y_ + rowHeight_; }

private:
  float width_, x_{}, y_, rowHeight_;
};
} // namespace drumfoundry::ui
