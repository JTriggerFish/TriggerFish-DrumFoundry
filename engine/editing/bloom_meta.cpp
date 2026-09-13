#include "meta.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace drumfoundry::editing {
MetaEdit BloomTiming(const Document &baseline, double position) {
  if (!std::isfinite(position) || position < -1 || position > 1)
    throw std::invalid_argument("Bloom timing must be between -1 and 1");
  MetaEdit edit;
  edit.values = {
      {"bloom_rate", baseline.Value("bloom_rate") * std::exp2(-position)},
      {"body_brightness", baseline.Value("body_brightness") - 6 * position},
      {"body_excitation_centre",
       baseline.Value("body_excitation_centre") * std::exp2(-.25 * position)}};
  for (auto &[key, value] : edit.values) {
    const auto &p = baseline.Description(key);
    const double bounded = std::clamp(value, p.minimum, p.maximum);
    if (bounded != value)
      edit.limited.push_back(ControlName(p));
    value = bounded;
  }
  return edit;
}
} // namespace drumfoundry::editing
