#include "document.hpp"
#include "runtime/voice.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace drumfoundry::editing {
void Document::Load(Json document) {
  Voice validated(48000, std::move(document));
  std::vector<Parameter> parameters;
  for (const auto &p : validated.Descriptors())
    parameters.push_back({p.at("key"), p.at("name"), p.at("unit"),
                          p.at("owner"), p.at("minimum"), p.at("maximum"),
                          p.at("default"), p.at("scale")});
  auto next = validated.Document();
  auto recipe = Instrument(next).at("recipe").get<std::string>();
  document_ = std::move(next);
  parameters_ = std::move(parameters);
  recipe_ = std::move(recipe);
}
const Parameter &Document::Description(const std::string &key) const {
  auto found = std::find_if(parameters_.begin(), parameters_.end(),
                            [&](const auto &p) { return p.key == key; });
  if (found == parameters_.end())
    throw std::invalid_argument("Unknown parameter: " + key);
  return *found;
}
double Document::Value(const std::string &key) const {
  const auto &p = Description(key);
  const auto &patch =
      document_.contains("instrument") ? document_.at("instrument") : document_;
  for (const auto &node : patch.at("nodes"))
    if (node.at("id") == p.owner)
      return node.at("parameters").at(key).get<double>();
  throw std::logic_error("Missing parameter owner: " + p.owner);
}
void Document::Set(const std::string &key, double value) {
  const auto &p = Description(key);
  if (!std::isfinite(value) || float(value) < float(p.minimum) ||
      float(value) > float(p.maximum) ||
      (p.scale >= 2 && value != std::floor(value)))
    throw std::invalid_argument("Invalid value for " + key);
  for (auto &node : Instrument(document_).at("nodes"))
    if (node.at("id") == p.owner) {
      node["parameters"][key] = value;
      return;
    }
  throw std::logic_error("Missing parameter owner: " + p.owner);
}
} // namespace drumfoundry::editing
