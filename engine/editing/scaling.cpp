#include "document.hpp"
#include <algorithm>
#include <cmath>

namespace drumfoundry::editing {
namespace {
double LogOffset(const std::string &key) {
  if (key == "field_turbulence")
    return .01;
  if (key == "field_wander_hz" || key == "field_doublet_split")
    return .1;
  return 0;
}
double Power(const std::string &key) {
  if (key == "bloom_energy_acceleration")
    return 3;
  if (key == "bloom_energy_sensitivity" || key == "field_motion_depth" ||
      key == "field_phase_bandwidth" || key == "field_beat_depth")
    return 2;
  return 1;
}
} // namespace
double Position(const Parameter &p, double value) {
  value = std::clamp(value, p.minimum, p.maximum);
  if (p.key == "bloom_rate")
    return value <= 0 ? 0
                      : std::clamp(.02 + .98 * std::log(value / .01) /
                                             std::log(p.maximum / .01),
                                   0., 1.);
  if (const auto offset = LogOffset(p.key); offset > 0)
    return std::log1p(value / offset) / std::log1p(p.maximum / offset);
  if (p.scale == 1)
    return std::log(value / p.minimum) / std::log(p.maximum / p.minimum);
  return std::pow((value - p.minimum) / (p.maximum - p.minimum),
                  1 / Power(p.key));
}
double ValueAt(const Parameter &p, double position) {
  position = std::clamp(position, 0., 1.);
  if (p.key == "bloom_rate")
    return position < .01
               ? 0
               : .01 * std::pow(p.maximum / .01, (position - .02) / .98);
  if (const auto offset = LogOffset(p.key); offset > 0)
    return offset * std::expm1(position * std::log1p(p.maximum / offset));
  if (p.scale == 1)
    return p.minimum * std::pow(p.maximum / p.minimum, position);
  const auto value =
      p.minimum + (p.maximum - p.minimum) * std::pow(position, Power(p.key));
  return p.scale >= 2 ? std::round(value) : value;
}
} // namespace drumfoundry::editing
