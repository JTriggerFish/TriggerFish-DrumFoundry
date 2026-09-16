#pragma once
#include "patch/document.hpp"
#include <string_view>

namespace drumfoundry {
// One classifier shared by native UI, audio runtime and CLAP metadata.
bool IsLiveParameter(std::string_view recipe, std::string_view key);
// Live in the editor via off-thread preparation, not sample-timed automation.
bool IsPreparedLiveParameter(std::string_view recipe, std::string_view key);
// Compare validated documents, rejecting structural edits. Validate values
// without allocating/preparing any DSP object. Main thread only.
bool ValidateLiveEdit(const Json &before, const Json &next,
                      bool allowPrepared = false);
bool ValidLiveDecay(const CrashMacroValues &) noexcept;
} // namespace drumfoundry
