#pragma once
#include "document.hpp"
namespace drumfoundry::editing {
struct Route {
  std::string id, from, to;
  bool enabled{}, required{};
};
std::vector<Route> Routes(const Document &);
Json NodePositions(const Json &document);
// Main-thread presentation edit. Only validated x/y fields can change.
void ApplyNodePositions(Json &document, const Json &positions);
} // namespace drumfoundry::editing
