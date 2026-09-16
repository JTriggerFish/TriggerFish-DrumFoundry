#include "live_controls.hpp"
#include <cmath>
#include <stdexcept>

namespace drumfoundry {
namespace {
bool Starts(std::string_view text, std::string_view prefix) {
  return text.substr(0, prefix.size()) == prefix;
}
} // namespace
bool IsLiveParameter(std::string_view recipe, std::string_view key) {
  if (Starts(key, "output_") || key == "model_level_db")
    return true;
  if (recipe == "metal.cymbal.v1")
    return Starts(key, "impact_") || Starts(key, "contact_") ||
           Starts(key, "bloom_") || Starts(key, "body_decay_") ||
           key == "direct_gain" || key == "field_gain" ||
           key == "body_excitation" || key == "velocity_brightness";
  return Starts(key, "contact_") || Starts(key, "fm_") ||
         Starts(key, "thump_") || key == "pitch_drop_octaves" ||
         key == "direct_level" || key == "body_level" || key == "wire_level" ||
         key == "resonance_level" || Starts(key, "resonance_decay_") ||
         key == "decay_seconds" || key == "decay_tilt" ||
         key == "ring_decay_seconds" || Starts(key, "tension_");
}

bool ValidLiveDecay(const CrashMacroValues &values) noexcept {
  const auto upper = values[std::size_t(CrashMacro::BodyDecayMaximumFrequency)];
  for (std::size_t k = 0; k < BodyDecayInteriorPointCount; ++k)
    if (values[std::size_t(CrashMacro::BodyDecayActiveFirst) + k] >= .5f &&
        values[std::size_t(CrashMacro::BodyDecayFrequencyFirst) + k] > upper)
      return false;
  return true;
}

bool ValidateLiveEdit(const Json &before, const Json &next) {
  auto previous = before;
  auto &oldPatch = Instrument(previous);
  const auto &patch =
      next.contains("instrument") ? next.at("instrument") : next;
  if (oldPatch.at("recipe") != patch.at("recipe"))
    return false;
  const auto recipe = patch.at("recipe").get<std::string>();
  auto topology = patch;
  if (topology.at("nodes").size() != oldPatch.at("nodes").size())
    return false;
  for (std::size_t i = 0; i < topology.at("nodes").size(); ++i) {
    auto &p = topology["nodes"][i]["parameters"];
    const auto &old = oldPatch.at("nodes")[i].at("parameters");
    if (!p.is_object() || p.size() != old.size())
      return false;
    for (auto &[key, value] : p.items()) {
      if (!old.contains(key))
        return false;
      if (value == old.at(key))
        continue;
      if (!IsLiveParameter(recipe, key))
        return false;
      const auto kind = ParseRecipe(recipe);
      const ParameterDescriptor *description = nullptr;
      for (std::size_t k = 0; k < detail::ParameterCount(kind); ++k)
        if (detail::Description(kind, k)->key == key)
          description = detail::Description(kind, k);
      if (!description || !value.is_number())
        throw std::invalid_argument("Invalid live parameter: " + key);
      const auto v = value.get<double>();
      if (!std::isfinite(v) || float(v) < description->minimum ||
          float(v) > description->maximum ||
          (int(description->scale) >= 2 && v != std::floor(v)))
        throw std::invalid_argument("Invalid live parameter: " + key);
      value = old.at(key);
    }
  }
  if (topology != oldPatch)
    return false;
  ValidateEnvelope(next);
  for (const auto &node : patch.at("nodes")) {
    const auto &p = node.at("parameters");
    if (!p.contains("body_decay_frequency_7"))
      continue;
    for (int k = 1; k < 7; ++k)
      if (p.at("body_decay_active_" + std::to_string(k)).get<double>() >= .5 &&
          p.at("body_decay_frequency_" + std::to_string(k)) >
              p.at("body_decay_frequency_7"))
        throw std::invalid_argument(
            "Active decay knots must not exceed the upper endpoint");
  }
  return true;
}
} // namespace drumfoundry
