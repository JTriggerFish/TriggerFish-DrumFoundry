#include "access.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace drumfoundry {
void SetParameter(detail::Session &s, std::size_t index, double value) {
  const auto *d = detail::Description(s, index);
  // Validate at the DSP's float precision: decimal JSON endpoints (e.g. .08)
  // need not have the exact double representation of a float descriptor.
  if (!d || !std::isfinite(value) ||
      std::abs(value) > std::numeric_limits<float>::max())
    throw std::invalid_argument("Invalid parameter: " + (d ? d->key : "index"));
  const auto v = static_cast<float>(value);
  if (v < d->minimum || v > d->maximum ||
      ((d->scale == ParameterScale::Choice ||
        d->scale == ParameterScale::Boolean) &&
       value != std::round(value)))
    throw std::invalid_argument("Invalid parameter: " + (d ? d->key : "index"));
  switch (s.recipe) {
  case detail::Recipe::MetallicPlate:
    s.crashValues[index] = v;
    break;
  case detail::Recipe::Kick:
    s.kickValues[index] = v;
    break;
  case detail::Recipe::MembraneDrum:
    s.membraneValues[index] = v;
    break;
  case detail::Recipe::SnareDrum:
    s.snareValues[index] = v;
    break;
  default:
    throw std::invalid_argument("Invalid recipe");
  }
}

void SetRoute(detail::Session &s, std::size_t index, bool enabled) {
  switch (s.recipe) {
  case detail::Recipe::MetallicPlate:
    s.cymbalRouting.enabled.at(index) = enabled;
    break;
  case detail::Recipe::Kick:
    s.kickRouting.enabled.at(index) = enabled;
    break;
  case detail::Recipe::MembraneDrum:
    s.membraneRouting.enabled.at(index) = enabled;
    break;
  case detail::Recipe::SnareDrum:
    s.snareRouting.enabled.at(index) = enabled;
    break;
  default:
    throw std::invalid_argument("Invalid recipe");
  }
}
} // namespace drumfoundry
