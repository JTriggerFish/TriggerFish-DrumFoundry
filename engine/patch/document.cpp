#include "document.hpp"
#include "parameters/access.hpp"
#include <map>
#include <set>
#include <stdexcept>

namespace drumfoundry {
Json &Instrument(Json &document) {
  return document.at("schema") == "triggerfish.percussion.fit/v1"
             ? document.at("instrument")
             : document;
}
Json ParseJson(const char *text) {
  if (!text)
    throw std::invalid_argument("JSON text is null");
  // Duplicate keys must not silently replace a parameter during import.
  std::vector<std::set<std::string>> keys;
  return Json::parse(text, [&](int, Json::parse_event_t event, Json &value) {
    if (event == Json::parse_event_t::object_start)
      keys.emplace_back();
    if (event == Json::parse_event_t::object_end)
      keys.pop_back();
    if (event == Json::parse_event_t::key &&
        !keys.back().insert(value.get<std::string>()).second)
      throw std::invalid_argument("Duplicate JSON key: " +
                                  value.get<std::string>());
    return true;
  });
}
void ApplyPatch(detail::Session &session, const Json &patch) {
  ValidateTopology(patch, session.recipe);
  std::map<std::string, std::size_t> indices;
  for (std::size_t i = 0; i < detail::ParameterCount(session); ++i)
    indices.emplace(detail::Description(session, i)->key, i);
  for (const auto &node : patch.at("nodes")) {
    if (!node.at("parameters").is_object())
      throw std::invalid_argument("Invalid parameters object");
    for (const auto &[key, value] : node.at("parameters").items()) {
      const auto found = indices.find(key);
      if (found == indices.end() ||
          ParameterOwner(session.recipe, key) !=
              node.at("id").get<std::string>() ||
          !value.is_number())
        throw std::invalid_argument("Unknown or misplaced parameter: " + key);
      SetParameter(session, found->second, value.get<double>());
    }
  }
  std::size_t index = 0;
  for (const auto &expected : Topology(session.recipe).at("connections")) {
    if (expected.at("required").get<bool>())
      continue;
    for (const auto &edge : patch.at("connections"))
      if (edge.at("from") == expected.at("from") &&
          edge.at("to") == expected.at("to"))
        SetRoute(session, index++, edge.at("enabled").get<bool>());
  }
}
Json DescribeParameters(const detail::Session &s) {
  auto result = Json::array();
  for (std::size_t i = 0; i < detail::ParameterCount(s); ++i) {
    const auto &d = *detail::Description(s, i);
    result.push_back({{"key", d.key},
                      {"name", d.name},
                      {"unit", d.unit},
                      {"minimum", d.minimum},
                      {"maximum", d.maximum},
                      {"default", d.defaultValue},
                      {"scale", static_cast<int>(d.scale)},
                      {"owner", ParameterOwner(s.recipe, d.key)}});
  }
  return result;
}
Json DefaultPatch(const std::string &key) {
  auto session = std::make_unique<detail::Session>();
  detail::Initialize(*session, ParseRecipe(key), 48000.f);
  auto patch = Topology(session->recipe);
  patch["schema"] = "triggerfish.percussion.patch/v1";
  patch["id"] = "default." + key;
  patch["name"] = "DrumFoundry default";
  patch["engineMinimum"] = 1;
  patch["outputs"] = {{"mono", patch["connections"].back()["to"]}};
  for (auto &node : patch["nodes"])
    node["parameters"] = Json::object();
  for (const auto &d : DescribeParameters(*session))
    for (auto &node : patch["nodes"])
      if (node["id"] == d["owner"])
        node["parameters"][d["key"].get<std::string>()] = d["default"];
  for (std::size_t i = 0; i < patch["connections"].size(); ++i) {
    patch["connections"][i]["id"] = "route-" + std::to_string(i);
    patch["connections"][i]["enabled"] = true;
  }
  return patch;
}
} // namespace drumfoundry
