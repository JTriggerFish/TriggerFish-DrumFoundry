#include "routes.hpp"
#include "patch/document.hpp"
#include "patch/modules.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace drumfoundry::editing {
namespace {
const Json &Patch(const Json &d) {
  return d.contains("instrument") ? d.at("instrument") : d;
}
} // namespace
std::vector<Route> Routes(const Document &d) {
  const auto &patch = Patch(d.JsonValue());
  const auto &contract = Topology(ParseRecipe(d.Recipe()));
  std::vector<Route> result;
  for (const auto &edge : patch.at("connections")) {
    bool required = false;
    for (const auto &expected : contract.at("connections"))
      if (expected.at("from") == edge.at("from") &&
          expected.at("to") == edge.at("to"))
        required = expected.at("required");
    result.push_back({edge.at("id"), edge.at("from"), edge.at("to"),
                      edge.at("enabled"), required});
  }
  for (const auto &attachment : patch.value("attachments", Json::array()))
    result.push_back({"attachment." + attachment.at("module").get<std::string>(),
        attachment.at("module").get<std::string>() + ".state",
        attachment.at("body").get<std::string>() + ".state",
        d.Value("hat_contact_enabled") >= .5, true, true});
  return result;
}
void Document::SetRoute(const std::string &id, bool enabled) {
  auto next = document_;
  auto &patch = Instrument(next);
  auto &edges = patch.at("connections");
  const auto edge =
      std::find_if(edges.begin(), edges.end(),
                   [&](const auto &e) { return e.at("id") == id; });
  if (edge == edges.end())
    throw std::invalid_argument("Unknown route: " + id);
  (*edge)["enabled"] = enabled;
  ValidateTopology(patch, ParseRecipe(recipe_));
  document_ =
      std::move(next); // Invalid/disconnected edits leave the document intact.
}
Json NodePositions(const Json &document) {
  Json positions = Json::object();
  unsigned i = 0;
  bool autoPlaceRim = false;
  for (const auto &node : Patch(document).at("nodes")) {
    const auto p = node.value("editor", Json::object());
    positions[node.at("id").get<std::string>()] = {
        {"x", p.value("x", 24. + 160 * (i % 5))},
        {"y", p.value("y", 22. + 70 * (i / 5))}};
    ++i;
    autoPlaceRim |= node.at("id") == RimContactId && !node.contains("editor");
  }
  if (autoPlaceRim) {
    // Put attachments beneath the audio path rather than drawing a long
    // backwards cable through every output node. Saved layouts take priority.
    double bottom = 22;
    for (const auto &[id, p] : positions.items())
      if (id != RimContactId) bottom = std::max(bottom, p.at("y").get<double>());
    const auto target = ResonatorId(ParseRecipe(Patch(document).at("recipe")));
    positions[RimContactId] = {{"x", positions.at(target).at("x")}, {"y", bottom + 70}};
  }
  return positions;
}
void ApplyNodePositions(Json &document, const Json &positions) {
  auto &nodes = Instrument(document).at("nodes");
  if (!positions.is_object() || positions.size() != nodes.size())
    throw std::invalid_argument("Layout must contain exactly the patch nodes");
  for (const auto &node : nodes) {
    if (node.contains("editor") && !node.at("editor").is_object())
      throw std::invalid_argument("Node editor metadata must be an object");
    const auto &p = positions.at(node.at("id").get<std::string>());
    if (!p.is_object() || p.size() != 2 || !p.at("x").is_number() ||
        !p.at("y").is_number())
      throw std::invalid_argument("Layout contains fields other than x/y");
    const double x = p.at("x"), y = p.at("y");
    if (!std::isfinite(x) || !std::isfinite(y) || x < 0 || x > 4096 || y < 0 ||
        y > 2048)
      throw std::invalid_argument("Node layout lies outside the design area");
  }
  for (auto &node : nodes) {
    const auto &p = positions.at(node.at("id").get<std::string>());
    node["editor"]["x"] = p.at("x");
    node["editor"]["y"] = p.at("y");
  }
}
void Document::MoveNode(const std::string &id, double x, double y) {
  auto positions = NodePositions(document_);
  if (!positions.contains(id))
    throw std::invalid_argument("Unknown layout node: " + id);
  positions[id] = {{"x", x}, {"y", y}};
  ApplyNodePositions(document_, positions);
}
} // namespace drumfoundry::editing
