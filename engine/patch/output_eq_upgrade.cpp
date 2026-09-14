#include "document.hpp"
#include <stdexcept>

namespace drumfoundry {
// Read older saved drum fits without retaining inactive multiband controls.
// An active multiband curve cannot be represented losslessly: reject
// explicitly.
void UpgradeOutputEq(Json &patch) {
  if (patch.at("recipe") == "metal.cymbal.v1")
    return;
  for (auto &node : patch.at("nodes")) {
    if (!node.contains("parameters"))
      continue;
    auto &p = node.at("parameters");
    if (!p.contains("equalizer_mode"))
      continue;
    const auto mode = p.at("equalizer_mode");
    if (mode != 0 && mode != 1)
      throw std::invalid_argument(
          "This saved fit uses the retired multiband EQ. Select bypass or "
          "radiation in the older version before importing.");
    const std::pair<const char *, const char *> names[]{
        {"equalizer_mode", "output_eq_enabled"},
        {"low_cut_hz", "output_low_cut"},
        {"high_cut_hz", "output_high_cut"},
        {"colour_frequency_hz", "output_colour_frequency"},
        {"colour_gain_db", "output_colour_gain"}};
    for (auto [oldKey, newKey] : names) {
      if (p.contains(newKey))
        throw std::invalid_argument("Duplicate old/new output EQ controls");
      if (p.contains(oldKey)) {
        p[newKey] = p.at(oldKey);
        p.erase(oldKey);
      }
    }
    for (int i = 1; i <= 4; ++i) {
      const auto prefix = "band_" + std::to_string(i);
      p.erase(prefix + "_frequency_hz");
      p.erase(prefix + "_gain_db");
    }
  }
}
} // namespace drumfoundry
