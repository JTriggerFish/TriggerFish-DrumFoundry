#pragma once
#include "document.hpp"
namespace drumfoundry::editing {
struct MetaEdit {
  std::vector<std::pair<std::string, double>> values;
  std::vector<std::string> limited;
};
// Design-time gestures only: return ordinary visible controls, no runtime
// state.
MetaEdit BloomTiming(const Document &baseline, double position);
MetaEdit SizeMeta(const Document &, double position);
} // namespace drumfoundry::editing
