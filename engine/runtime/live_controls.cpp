#include "live_controls.hpp"
#include "parameters/validation.hpp"
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
  return ValidDecayEndpoints(
      upper,
      [&](unsigned i) {
        return values[std::size_t(CrashMacro::BodyDecayFrequencyFirst) + i - 1];
      },
      [&](unsigned i) {
        return values[std::size_t(CrashMacro::BodyDecayActiveFirst) + i - 1];
      });
}

bool IsPreparedLiveParameter(std::string_view recipe, std::string_view key) {
  if (recipe == "metal.cymbal.v1")
    return (Starts(key, "resolved_") || Starts(key, "field_") ||
            key == "body_tune" || key == "body_brightness" ||
            key == "body_excitation_centre");
  return Starts(key, "resonance_frequency_") ||
         Starts(key, "resonance_level_") || key == "fundamental_hz" ||
         key == "inharmonicity" || key == "body_brightness";
}

bool ValidateLiveEdit(const Json &before, const Json &next,
                      bool allowPrepared) {
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
      if (!IsLiveParameter(recipe, key) &&
          !(allowPrepared && IsPreparedLiveParameter(recipe, key)))
        return false;
      const auto kind = ParseRecipe(recipe);
      const ParameterDescriptor *description = nullptr;
      for (std::size_t k = 0; k < detail::ParameterCount(kind); ++k)
        if (detail::Description(kind, k)->key == key)
          description = detail::Description(kind, k);
      if (!description || !value.is_number())
        throw std::invalid_argument("Invalid live parameter: " + key);
      const auto v = value.get<double>();
      if (!ValidParameterValue(*description, v))
        throw std::invalid_argument("Invalid live parameter: " + key);
      value = old.at(key);
    }
  }
  if (topology != oldPatch)
    return false;
  ValidateEnvelope(next);
  for (const auto &node : patch.at("nodes"))
    ValidateDecayParameters(node.at("parameters"));
  return true;
}
} // namespace drumfoundry
