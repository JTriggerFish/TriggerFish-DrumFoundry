#include "access.hpp"
#include "validation.hpp"
#include <stdexcept>

namespace drumfoundry {
void SetParameter(detail::Session &s, std::size_t index, double value) {
  const auto *d = detail::Description(s, index);
  if (!d || !ValidParameterValue(*d, value))
    throw std::invalid_argument("Invalid parameter: " + (d ? d->key : "index"));
  const auto v = static_cast<float>(value);
  if (s.recipe != detail::Recipe::MetallicPlate &&
      index >= detail::RimParameterFirst(s.recipe)) {
    s.rimValues[index - detail::RimParameterFirst(s.recipe)] = v;
    return;
  }
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
