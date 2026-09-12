#include "parameters/access.hpp"

namespace drumfoundry {
namespace {
bool Starts(std::string_view key, std::string_view prefix) {
  return key.substr(0, prefix.size()) == prefix;
}
std::string_view MetalOwner(std::string_view k) {
  if (k == "model_level_db")
    return "output";
  if (Starts(k, "output_") || k == "direct_gain" || k == "field_gain")
    return "observation";
  if (Starts(k, "impact_") || k == "velocity_brightness")
    return "contact";
  return "body";
}
std::string_view MembraneOwner(std::string_view k) {
  if (k == "model_level_db")
    return "membrane-output";
  if (k == "contact_direct_level" || k == "fm_direct_level")
    return "membrane-direct-mix";
  if (k == "contact_body_level" || k == "fm_body_level")
    return "membrane-body-mix";
  if (Starts(k, "contact_"))
    return "membrane-contact";
  if (Starts(k, "fm_") || k == "pitch_drop_octaves")
    return "membrane-fm";
  if (Starts(k, "tension_"))
    return "membrane-tension";
  if (k == "fundamental_hz" || k == "decay_seconds" || k == "decay_tilt" ||
      k == "inharmonicity" || k == "body_brightness")
    return "membrane-body";
  if (k == "direct_level" || k == "body_level" || k == "direct_delay_ms")
    return "membrane-observation";
  return "membrane-eq";
}
} // namespace
std::string_view ParameterOwner(detail::Recipe recipe, std::string_view k) {
  if (recipe == detail::Recipe::MetallicPlate)
    return MetalOwner(k);
  if (recipe == detail::Recipe::Kick) {
    if (k == "model_level_db")
      return "kick-output";
    if (Starts(k, "contact_"))
      return "kick-contact";
    if (Starts(k, "thump_"))
      return "kick-thump";
    if (Starts(k, "resonance_"))
      return "kick-resonance";
    if (Starts(k, "tension_"))
      return "kick-tension";
    return "kick-observation";
  }
  if (recipe == detail::Recipe::SnareDrum) {
    if (Starts(k, "wire_"))
      return "snare-wires";
    if (Starts(k, "ring_"))
      return "membrane-body";
  }
  return MembraneOwner(k);
}
} // namespace drumfoundry
