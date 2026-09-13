#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace drumfoundry::editing {
using Json = nlohmann::json;
struct Parameter {
  std::string key, name, unit, owner;
  double minimum{}, maximum{}, initial{};
  int scale{};
};
// Main-thread editable document, shared by native UI and offline development.
// Load validates transactionally with the actual C++ voice. Scalar edits use
// that voice's authoritative descriptors; synthesis never reads this object.
class Document {
public:
  void Load(Json document);
  const Json &JsonValue() const { return document_; }
  const std::vector<Parameter> &Parameters() const { return parameters_; }
  const Parameter &Description(const std::string &key) const;
  double Value(const std::string &key) const;
  void Set(const std::string &key, double value);
  void SetMany(const std::vector<std::pair<std::string, double>> &values);
  const std::string &Recipe() const { return recipe_; }

private:
  Json document_;
  std::vector<Parameter> parameters_;
  std::string recipe_;
};
// Preserve the web workbench's useful low-range tapers, in one native mapping.
double Position(const Parameter &, double value);
double ValueAt(const Parameter &, double position);
std::string Section(const Parameter &);
bool RightColumn(const Parameter &);
std::string ControlName(const Parameter &);
std::string ChoiceName(const Parameter &, int value);
} // namespace drumfoundry::editing
