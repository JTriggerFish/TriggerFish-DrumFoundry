#pragma once
#include "crash_macros.hpp"
#include "tfdsp/percussion/modal_rim_contact.hpp"

namespace drumfoundry {
// Historical hat_* keys/indices remain stable for saved fits and automation.
// All recipes use these same seven descriptors and the same DSP mapping.
inline constexpr std::size_t RimControlFirst = std::size_t(CrashMacro::HatContactEnabled);
inline constexpr std::size_t RimControlCount = 7;
static_assert(RimControlFirst + RimControlCount == CrashMacroCount,
              "Optional controls must stay last: native descriptor indices are stable");
using RimControlValues = std::array<float, RimControlCount>;
inline RimControlValues DefaultRimControls() noexcept {
  RimControlValues result{};
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i] = CrashMacroDescription(RimControlFirst + i).defaultValue;
  return result;
}
inline tfdsp::percussion::ModalRimContactParameters
ApplyRimControls(const float *v) noexcept {
  return {v[0] >= .5f, v[1], v[2], v[3], v[4], v[5], v[6]};
}
} // namespace drumfoundry
