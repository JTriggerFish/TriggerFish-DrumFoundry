#include "editing/routes.hpp"
#include <stdexcept>
namespace {
void Check(bool value) {
  if (!value)
    throw std::runtime_error("Routing edit regression");
}
} // namespace
void RouteTests(drumfoundry::editing::Document d) {
  using namespace drumfoundry::editing;
  const auto before = d.JsonValue();
  const auto routes = Routes(d);
  Check(!routes.empty());
  for (const auto &route : routes) {
    d.SetRoute(route.id, route.enabled);
    Check(d.JsonValue() == before);
    if (route.required) {
      bool rejected = false;
      try {
        d.SetRoute(route.id, false);
      } catch (const std::invalid_argument &) {
        rejected = true;
      }
      Check(rejected && d.JsonValue() == before);
    }
  }
  auto positions = NodePositions(before);
  const auto id = positions.begin().key();
  positions[id] = {{"x", 80}, {"y", 90}};
  auto next = before;
  ApplyNodePositions(next, positions);
  d.MoveNode(id, 80, 90);
  Check(d.JsonValue() == next);
  const auto moved = d.JsonValue();
  bool rejected = false;
  positions[id]["parameters"] = {{"hiddenGain", 10}};
  try {
    ApplyNodePositions(next, positions);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  Check(rejected && next == moved);
  // Disable every optional route. The final audible-path removal must fail.
  rejected = false;
  for (const auto &route : routes) {
    if (route.required)
      continue;
    const auto previous = d.JsonValue();
    try {
      d.SetRoute(route.id, false);
    } catch (const std::invalid_argument &) {
      Check(d.JsonValue() == previous);
      rejected = true;
    }
  }
  Check(rejected);
}
