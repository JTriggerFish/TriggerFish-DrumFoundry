#pragma once
#include "runtime/live_controls.hpp"
#include <clap/clap.h>
#include <vector>

namespace drumfoundry::clap_adapter {
inline constexpr std::size_t DesignCapacity = 256;
struct DesignParameter {
  clap_id id{};
  detail::Recipe recipe{};
  std::size_t index{};
  const ParameterDescriptor *descriptor{};
  std::string owner, module;
};
// Built/warmed on the main thread. IDs depend only on recipe/key, not
// positions.
const std::vector<DesignParameter> &DesignParameters();
const DesignParameter *FindDesignParameter(clap_id) noexcept;
std::size_t DesignSlot(clap_id) noexcept;
bool ValidDesignValue(const DesignParameter &, double) noexcept;
// A valid coupled curve must also stay valid through its scalar setters.
int DesignEditPriority(const DesignParameter &, double value,
                       double current) noexcept;
} // namespace drumfoundry::clap_adapter
