#include "document.hpp"
#include <map>

namespace drumfoundry::editing {
std::string ControlName(const Parameter &p) {
  static const std::map<std::string, std::string> names{
      {"field_turbulence", "Surrounding rings at 1 kHz"},
      {"field_turbulence_slope", "Bass / treble balance"},
      {"field_packet_spread", "Spread"},
      {"field_satellite_density", "Density"},
      {"field_doublet_split", "Speed at 125 Hz"},
      {"field_beat_depth", "Depth"},
      {"field_beat_rate_tilt", "Treble speed scaling"},
      {"field_wander_hz", "Amount"},
      {"field_wander_rate", "Speed"},
      {"field_motion_depth", "Amount"},
      {"field_motion_rate", "Speed"},
      {"field_motion_sharing", "Shimmer moves together"},
      {"field_phase_bandwidth", "Amount"},
      {"field_phase_tilt", "Bass / treble balance"},
      {"body_tune", "Tuning"}};
  const auto found = names.find(p.key);
  return found == names.end() ? p.name : found->second;
}
std::string ChoiceName(const Parameter &p, int value) {
  if (p.scale == 2)
    return value ? "On" : "Off";
  if (p.key == "field_distribution") {
    static const char *names[]{"Scattered", "Even coverage", "Beating doublets",
                               "Paired rings", "Plate cloud"};
    if (value >= 0 && value < 5)
      return names[value];
  }
  return std::to_string(value);
}
} // namespace drumfoundry::editing
