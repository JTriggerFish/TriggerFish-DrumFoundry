#pragma once
#include "solver.hpp"
namespace drumfoundry::decay_hold {
struct Axis {
  std::string key;
  double origin, lo, hi, step, maxStep, minimum, maximum;
  bool logarithmic;
  double Decode(double x) const;
};
std::vector<Axis> Coordinates(const Document &, const Document &);
} // namespace drumfoundry::decay_hold
