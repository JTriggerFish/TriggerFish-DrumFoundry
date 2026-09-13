#pragma once
#include <map>
#include <string>
namespace drumfoundry::editing {
// Explicit design-tool endpoints ported from web
// size_meta/metallic_calibrations. These are not built-in fits, DSP defaults,
// or coefficients hidden in the voice.
using SizeValues = std::map<std::string, double>;
SizeValues LargeSizeEndpoint();
SizeValues SmallSizeEndpoint();
} // namespace drumfoundry::editing
