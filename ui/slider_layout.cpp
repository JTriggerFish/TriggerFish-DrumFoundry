#include "controls.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace drumfoundry::ui {
std::string Slider::Readout(double value) const {
  auto unit = unit_;
  if (unit == " Hz" && std::abs(value) >= 1000) {
    value /= 1000;
    unit = " kHz";
  }
  char text[48];
  const double magnitude = std::abs(value);
  if (magnitude > 0 && magnitude < .0001)
    std::snprintf(text, sizeof(text), "%.2g", value);
  else
    std::snprintf(text, sizeof(text), "%.*f",
                  magnitude >= 100   ? 1
                  : magnitude >= 1   ? 2
                  : magnitude >= .01 ? 3
                                     : 4,
                  value);
  std::string result = text;
  if (result.find('.') != std::string::npos &&
      result.find('e') == std::string::npos) {
    while (result.back() == '0')
      result.pop_back();
    if (result.back() == '.')
      result.pop_back();
  }
  return result + unit;
}
float Slider::PreferredHeight(float availableWidth) const {
  const auto font = FrameFont(*this);
  const auto measure = [&](const std::string &text) {
    return font.stringWidth(visage::String(text).toUtf32());
  };
  // Reserve endpoint widths too. Reflow happens on resize/text-size changes,
  // not on every drag, so the surrounding controls do not jump while editing.
  float valueWidth = 0;
  for (double value : {low_, high_, initial_, value_})
    valueWidth = std::max(valueWidth, measure(Readout(value)));
  return measure(label_) + 12 + valueWidth <= availableWidth ? 44.f : 66.f;
}
} // namespace drumfoundry::ui
