#pragma once
#include "runtime/session.hpp"
#include <string_view>

namespace drumfoundry {
// Off-thread parameter access. Reject invalid values rather than silently
// clamp.
void SetParameter(detail::Session &, std::size_t index, double value);
void SetRoute(detail::Session &, std::size_t index, bool enabled);
std::string_view ParameterOwner(detail::Recipe, std::string_view key);
} // namespace drumfoundry
