#pragma once
#include "patch/document.hpp"
#include <string_view>

namespace drumfoundry {
// One classifier shared by native UI, audio runtime and CLAP metadata.
bool IsLiveParameter(std::string_view recipe, std::string_view key);
// Compare validated documents, rejecting structural edits. Validate values
// without allocating/preparing any DSP object. Main thread only.
bool ValidateLiveEdit(const Json &before, const Json &next);
bool ValidLiveDecay(const CrashMacroValues &) noexcept;
} // namespace drumfoundry
