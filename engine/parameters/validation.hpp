#pragma once
#include "parameter_descriptor.hpp"
#include <cmath>
#include <limits>

namespace drumfoundry {
// Shared by JSON, editing and realtime automation. Decimal endpoints are
// compared at DSP float precision; discrete inputs must be exact integers.
inline bool ValidParameterValue(double value, float minimum, float maximum,
                                ParameterScale scale) noexcept {
  if (!std::isfinite(value) ||
      std::abs(value) > std::numeric_limits<float>::max())
    return false;
  const float v = static_cast<float>(value);
  return v >= minimum && v <= maximum &&
         ((scale != ParameterScale::Choice &&
           scale != ParameterScale::Boolean) ||
          value == std::floor(value));
}
inline bool ValidParameterValue(const ParameterDescriptor &d,
                                double value) noexcept {
  return ValidParameterValue(value, d.minimum, d.maximum, d.scale);
}

// Accessors bridge typed runtime arrays and main-thread JSON without copying
// either representation. Individual values have already passed scalar checks.
template <class Frequency, class Active>
bool ValidDecayEndpoints(float upper, Frequency frequency, Active active) {
  for (unsigned i = 1; i < 7; ++i)
    if (active(i) >= .5 && static_cast<float>(frequency(i)) > upper)
      return false;
  return true;
}
} // namespace drumfoundry
