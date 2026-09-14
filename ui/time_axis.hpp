#pragma once
#include <algorithm>
#include <vector>

namespace drumfoundry::ui {
struct TimeTick {
  double seconds;
  float x, labelX, labelWidth;
};

// Each pane owns its labels, including its starting time. This preserves the
// onset at arbitrary divider positions and avoids labels crossing the
// divider.
inline std::vector<TimeTick> TimeAxisTicks(float left, float width,
                                           float scale, double pan,
                                           double span, double split = 0,
                                           bool mirror = false) {
  std::vector<TimeTick> ticks;
  const auto pane = [&](float x, float w, bool reverse) {
    if (w <= 8)
      return;
    const float labelWidth = std::min(65 * scale, w - 8);
    const float spacing = std::max(85 * scale, 1.5f * labelWidth + 8);
    const int intervals = std::clamp(int(w / spacing), 1, 6);
    // In a tiny pane retain its starting time rather than overlapping labels.
    const int last = w < 2 * (labelWidth + 8) ? 0 : intervals;
    for (int i = 0; i <= last; ++i) {
      const double fraction = double(i) / intervals;
      const float position = x + w * float(reverse ? 1 - fraction : fraction);
      ticks.push_back({pan + span * fraction, position,
                       std::clamp(position - labelWidth / 2, x + 4,
                                  x + w - 4 - labelWidth),
                       labelWidth});
    }
  };
  if (split > 0 && split < 1) {
    const float a = width * float(split);
    pane(left, a, mirror);
    pane(left + a, width - a, false);
  } else
    pane(left, width, false);
  return ticks;
}
} // namespace drumfoundry::ui
