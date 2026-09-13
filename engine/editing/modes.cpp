#include "modes.hpp"
#include <stdexcept>

namespace drumfoundry::editing {
namespace {
void Append(std::vector<std::pair<std::string, double>> &values,
            const std::string &prefix, unsigned slot, Mode mode) {
  const auto suffix = std::to_string(slot);
  values.emplace_back(prefix + "frequency_" + suffix, mode.frequency);
  values.emplace_back(prefix + "level_" + suffix, mode.level);
  if (prefix == "resolved_") {
    values.emplace_back(prefix + "turbulence_" + suffix, mode.turbulence);
    values.emplace_back(prefix + "allocation_" + suffix, mode.allocation);
  }
}
} // namespace
std::string ModePrefix(const Document &d) {
  for (const auto &p : d.Parameters()) {
    if (p.key == "resolved_frequency_0")
      return "resolved_";
    if (p.key == "resonance_frequency_0")
      return "resonance_";
  }
  return "";
}
std::vector<Mode> Modes(const Document &d) {
  const auto prefix = ModePrefix(d);
  std::vector<Mode> result;
  if (prefix.empty())
    return result;
  for (const auto &p : d.Parameters()) {
    if (p.key.rfind(prefix + "frequency_", 0) != 0)
      continue;
    const auto suffix = p.key.substr((prefix + "frequency_").size());
    result.push_back(
        {d.Value(p.key), d.Value(prefix + "level_" + suffix),
         prefix == "resolved_" ? d.Value(prefix + "turbulence_" + suffix) : 1,
         prefix == "resolved_" ? d.Value(prefix + "allocation_" + suffix) : 1});
  }
  return result;
}
void SetMode(Document &d, unsigned slot, Mode mode) {
  std::vector<std::pair<std::string, double>> values;
  Append(values, ModePrefix(d), slot, mode);
  d.SetMany(values);
}
void ReplaceModes(Document &d, const std::vector<Mode> &modes) {
  const auto existing = Modes(d);
  if (modes.size() > existing.size())
    throw std::invalid_argument("Too many modal handles for this instrument");
  std::vector<std::pair<std::string, double>> values;
  const auto prefix = ModePrefix(d);
  for (unsigned i = 0; i < existing.size(); ++i) {
    auto mode = i < modes.size() ? modes[i] : existing[i];
    if (i >= modes.size())
      mode.level = -72;
    Append(values, prefix, i, mode);
  }
  d.SetMany(values);
}
unsigned InsertMode(Document &d, double frequency, double level) {
  const auto modes = Modes(d);
  for (unsigned i = 0; i < modes.size(); ++i)
    if (modes[i].level <= -72) {
      SetMode(d, i, {frequency, level, 1, 1});
      return i;
    }
  throw std::invalid_argument(
      "All modal handles are in use — remove one or clear the series first");
}
} // namespace drumfoundry::editing
