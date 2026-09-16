#pragma once
#include "parameters/crash_macros.hpp"
#include "patch/document.hpp"
#include <memory>

namespace drumfoundry {
// Fully designed off-thread. This is coefficient/state storage, not another
// instrument: no contact, filters, limiter, audio buffers or render pass.
struct PreparedMetallicEdit {
  tfdsp::percussion::CrashCymbalParameters parameters{};
  tfdsp::percussion::CrashModalField field;
};
struct PreparedModalEdit {
  detail::Recipe recipe{};
  CrashMacroValues
      values{}; // Capacity covers every recipe; only its prefix is used.
  std::unique_ptr<PreparedMetallicEdit> metallic;
  tfdsp::percussion::MembraneResonator<
      tfdsp::percussion::MembraneModeCount>::PreparedParameters membrane;
  float sampleRate{};
};
std::unique_ptr<PreparedModalEdit> PrepareModalEdit(float sampleRate,
                                                    Json next);
} // namespace drumfoundry
