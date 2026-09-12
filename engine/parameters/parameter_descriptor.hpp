#pragma once

#include <string>

namespace drumfoundry {

enum class ParameterScale : int { Linear, Logarithmic, Boolean, Choice };

struct ParameterDescriptor {
  std::string key;
  std::string name;
  std::string unit;
  float minimum;
  float maximum;
  float defaultValue;
  ParameterScale scale{ParameterScale::Linear};
};

} // namespace drumfoundry
