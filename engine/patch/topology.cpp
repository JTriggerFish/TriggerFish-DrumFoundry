#include "document.hpp"
#include "parameters/access.hpp"
#include "topology_data.hpp"
#include "modules.hpp"
#include <set>
#include <stdexcept>

namespace drumfoundry {
const Json &Topology(detail::Recipe recipe) {
  static const auto data = Json::parse(RecipeTopologyJson);
  return data.at(static_cast<std::size_t>(recipe));
}
detail::Recipe ParseRecipe(const std::string &key) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(detail::Recipe::Count);
       ++i)
    if (Topology(static_cast<detail::Recipe>(i)).at("recipe") == key)
      return static_cast<detail::Recipe>(i);
  throw std::invalid_argument("Unsupported recipe: " + key);
}
namespace {
void Require(bool valid, const char *message) {
  if (!valid)
    throw std::invalid_argument(message);
}
std::string Edge(const Json &j) {
  return j.at("from").get<std::string>() + ">" + j.at("to").get<std::string>();
}
bool Audible(detail::Recipe recipe, const std::vector<bool> &on) {
  if (recipe == detail::Recipe::MetallicPlate)
    return on[1] || (on[0] && on[2]);
  if (recipe == detail::Recipe::Kick)
    return on[0] || on[1] || on[2];
  const bool direct = on[0] || on[2], drive = on[1] || on[3];
  return direct ||
         (drive &&
          (on[4] || (recipe == detail::Recipe::SnareDrum && on[5] && on[6])));
}
} // namespace
void ValidateTopology(const Json &patch, detail::Recipe recipe) {
  const auto &expected = Topology(recipe);
  Require(patch.at("schema") == "triggerfish.percussion.patch/v1" &&
              patch.at("engineMinimum").is_number_integer() &&
              patch.at("engineMinimum") == 1,
          "Unsupported patch schema or engine version");
  Require(patch.at("nodes").is_array() && patch.at("connections").is_array() &&
              patch.at("nodes").size() == expected.at("nodes").size() +
                  (HasRimContact(patch) ? 1 : 0) &&
              patch.at("connections").size() ==
                  expected.at("connections").size(),
          "Unsupported recipe structure");
  std::set<std::string> ids;
  for (const auto &node : patch.at("nodes")) {
    const auto id = node.at("id").get<std::string>();
    Require(ids.insert(id).second, "Duplicate node ID");
    if (id == RimContactId) {
      Require(node.at("type") == RimContactType &&
                  node.at("version").is_number_integer() && node.at("version") == 1,
              "Unsupported rim contact type or version");
      continue;
    }
    const auto &nodes = expected.at("nodes");
    const auto found =
        std::find_if(nodes.begin(), nodes.end(),
                     [&](const auto &n) { return n.at("id") == id; });
    Require(found != nodes.end() && node.at("type") == found->at("type") &&
                node.at("version").is_number_integer() &&
                node.at("version") == 1,
            "Unsupported node type or version");
  }
  ValidateAttachments(patch, recipe);
  std::map<std::string, bool> edges;
  ids.clear();
  for (const auto &edge : patch.at("connections")) {
    Require(ids.insert(edge.at("id").get<std::string>()).second,
            "Duplicate connection ID");
    Require(edge.at("enabled").is_boolean() && !edge.contains("gain"),
            "Invalid route: expected only an enabled switch");
    Require(edges.emplace(Edge(edge), edge.at("enabled").get<bool>()).second,
            "Duplicate route");
  }
  std::vector<bool> enabled;
  for (const auto &edge : expected.at("connections")) {
    const auto found = edges.find(Edge(edge));
    Require(found != edges.end(), "Unsupported route endpoints");
    Require(!edge.at("required").get<bool>() || found->second,
            "Required route disabled");
    enabled.push_back(found->second);
  }
  Require(Audible(recipe, enabled), "Patch has no audible route");
  const auto output = expected.at("connections").back().at("to");
  Require(patch.at("outputs") == Json{{"mono", output}}, "Unsupported output");
}
} // namespace drumfoundry
