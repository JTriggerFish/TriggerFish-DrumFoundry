#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace drumfoundry::ui {
struct FrequencyTick {
  double frequency;
  float y, labelY;
};
// Always show the actual upper bound, including after zooming. Keep its
// label inside the pane and omit nearby ticks rather than overlapping text.
inline std::vector<FrequencyTick> FrequencyAxisTicks(double low, double high,
                                                     float top, float height,
                                                     float textHeight) {
  if (low <= 0 || high <= low || height < textHeight)
    return {};
  std::vector<FrequencyTick> ticks{{high, top, top}};
  for (double f : {20000., 15000., 10000., 5000., 2000., 1000., 500., 200.,
                   100., 50., 20.}) {
    if (f >= high || f < low)
      continue;
    const float y =
        top + float(std::log(high / f) / std::log(high / low)) * height;
    const float label =
        std::clamp(y - textHeight / 2, top, top + height - textHeight);
    if (label >= ticks.back().labelY + textHeight + 2)
      ticks.push_back({f, y, label});
  }
  return ticks;
}
} // namespace drumfoundry::ui
