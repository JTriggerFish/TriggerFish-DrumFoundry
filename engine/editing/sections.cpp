#include "document.hpp"
namespace drumfoundry::editing {
namespace {
bool Starts(const std::string &text, const char *prefix) {
  return text.rfind(prefix, 0) == 0;
}
} // namespace
std::string Section(const Parameter &p) {
  const auto &k = p.key;
  if (k == "hat_openness")
    return "Playing";
  if (Starts(k, "hat_"))
    return "Rim contact";
  if (p.owner == "observation" && Starts(k, "contact_noise_"))
    return "Strike accent";
  if (p.owner == "membrane-body")
    return "Membrane"; // Its brightness is not the metallic diffusion control.
  if (Starts(k, "body_decay_"))
    return "Modal T60";
  if (Starts(k, "resolved_") || Starts(k, "mode_"))
    return "Modal anchors";
  if (Starts(k, "resonance_frequency_") || Starts(k, "resonance_level_"))
    return "Modal anchors";
  if (Starts(k, "field_wander"))
    return "Slow detuning";
  if (Starts(k, "field_motion"))
    return "Shimmer";
  if (Starts(k, "field_phase"))
    return "Phase blur";
  if (Starts(k, "field_beat") || k == "field_doublet_split")
    return "Beating";
  if (Starts(k, "field_") && k != "field_gain")
    return "Packet texture";
  if (Starts(k, "bloom_") || k == "body_brightness" ||
      k == "body_excitation_centre")
    return "Bloom / energy travel";
  if (k == "body_excitation" || k == "body_tune")
    return "Resonance";
  if (Starts(k, "output_") || k == "model_level_db" || k == "field_gain" ||
      k == "direct_gain")
    return "Output";
  if (Starts(k, "impact_") || k == "velocity_brightness")
    return "Contact presentation";
  if (p.owner.find("contact") != std::string::npos)
    return "Contact";
  if (p.owner.find("thump") != std::string::npos)
    return "Thump";
  if (p.owner.find("resonance") != std::string::npos)
    return "Resonance";
  if (p.owner.find("wire") != std::string::npos)
    return "Snare wires";
  if (p.owner.find("tension") != std::string::npos)
    return "Strike / tension";
  if (p.owner.find("direct-mix") != std::string::npos)
    return "Direct mix";
  if (p.owner.find("body-mix") != std::string::npos)
    return "Body drive";
  if (p.owner.find("observation") != std::string::npos)
    return "Output";
  if (p.owner.find("-eq") != std::string::npos)
    return "Output";
  return p.owner;
}
bool RightColumn(const Parameter &p) {
  const auto section = Section(p);
  if (section == "Modal T60" || section == "Rim contact" || section == "Playing")
    return false;
  return section == "Bloom / energy travel" || section == "Resonance" ||
         section == "Thump" || section == "Strike / tension" ||
         section == "Packet texture" || section == "Beating" ||
         section == "Slow detuning" || section == "Shimmer" ||
         section == "Phase blur" || section == "Modal anchors" ||
         p.owner.find("body") != std::string::npos ||
         p.owner.find("wire") != std::string::npos;
}
} // namespace drumfoundry::editing
