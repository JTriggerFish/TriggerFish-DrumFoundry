#include "modules.hpp"
#include <stdexcept>

namespace drumfoundry {
bool HasRimContact(const Json &patch) {
  for (const auto &node : patch.at("nodes"))
    if (node.at("id") == RimContactId) return true;
  return false;
}
const char *ResonatorId(detail::Recipe recipe) {
  switch (recipe) {
  case detail::Recipe::MetallicPlate: return "body";
  case detail::Recipe::Kick: return "kick-resonance";
  case detail::Recipe::MembraneDrum:
  case detail::Recipe::SnareDrum: return "membrane-body";
  default: throw std::invalid_argument("No compatible resonator");
  }
}
Json RimContactNode() {
  return {{"id", RimContactId}, {"type", RimContactType}, {"version", 1},
          {"parameters", Json::object()}};
}
Json RimContactAttachment(detail::Recipe recipe) {
  return {{"module", RimContactId}, {"body", ResonatorId(recipe)},
          {"port", "modal-state"}};
}
void ValidateAttachments(const Json &patch, detail::Recipe recipe) {
  const auto attachments = patch.value("attachments", Json::array());
  const auto expected = HasRimContact(patch)
      ? Json::array({RimContactAttachment(recipe)}) : Json::array();
  if (attachments != expected)
    throw std::invalid_argument(
        "Rim contact requires exactly one modal-state attachment to this body's resonator");
}
void UpgradeRimContact(Json &patch) {
  if (patch.at("recipe") != "metal.cymbal.v1") return;
  auto module = RimContactNode();
  for (auto &node : patch.at("nodes")) {
    if (node.at("id") != "body" || !node.contains("parameters") ||
        !node.at("parameters").is_object()) continue;
    auto &parameters = node.at("parameters");
    for (auto it = parameters.begin(); it != parameters.end();) {
      if (it.key().rfind("hat_", 0) != 0) { ++it; continue; }
      module["parameters"][it.key()] = it.value();
      it = parameters.erase(it);
    }
  }
  if (module.at("parameters").empty()) return;
  if (HasRimContact(patch) || patch.contains("attachments"))
    throw std::invalid_argument("Conflicting old/new rim contact ownership");
  patch["nodes"].push_back(std::move(module));
  patch["attachments"] = Json::array({RimContactAttachment(detail::Recipe::MetallicPlate)});
}
} // namespace drumfoundry
