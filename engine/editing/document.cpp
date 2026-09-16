#include "document.hpp"
#include "parameters/validation.hpp"
#include "runtime/voice.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace drumfoundry::editing {
namespace {
void Validate(const Parameter &p, double value) {
  if (!ValidParameterValue(value, float(p.minimum), float(p.maximum),
                           static_cast<ParameterScale>(p.scale)))
    throw std::invalid_argument("Invalid value for " + p.key);
}
} // namespace
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
  Validate(p, value);
  for (auto &node : Instrument(document_).at("nodes"))
    if (node.at("id") == p.owner) {
      node["parameters"][key] = value;
      return;
    }
  throw std::logic_error("Missing parameter owner: " + p.owner);
}
void Document::SetMany(
    const std::vector<std::pair<std::string, double>> &values) {
  // Resolve and validate all destinations before any mutation. Scalar
  // assignment into existing JSON numbers does not copy the complete patch
  // during drags.
  std::vector<std::pair<Json *, double>> destinations;
  destinations.reserve(values.size());
  for (const auto &[key, value] : values) {
    const auto &p = Description(key);
    Validate(p, value);
    Json *destination = nullptr;
    for (auto &node : Instrument(document_).at("nodes"))
      if (node.at("id") == p.owner)
        destination = &node.at("parameters").at(key);
    if (!destination)
      throw std::logic_error("Missing parameter owner: " + p.owner);
    destinations.emplace_back(destination, value);
  }
  for (auto [destination, value] : destinations)
    *destination = value;
}
} // namespace drumfoundry::editing
