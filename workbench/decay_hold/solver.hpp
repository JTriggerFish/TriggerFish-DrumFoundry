#pragma once
#include "editing/document.hpp"
#include "measurement.hpp"
namespace drumfoundry::decay_hold {
using Document = editing::Document;
using MeasureDocument = std::function<Cells(const Document &, uint32_t seed)>;
struct Result {
  bool accepted{}, atLimit{};
  double before{}, after{}, front{};
  unsigned evaluations{};
  std::string reason;
  std::vector<std::pair<std::string, double>> values;
};
// Bounded design-time solve. Never changes gains, adds knots or alters DSP.
Result Compensate(const Document &baseline, const Document &edited,
                  uint32_t seed, const MeasureDocument &,
                  const Cancel &cancel = {},
                  const std::function<void(unsigned)> &progress = {});
// Only bloom/excitation edits qualify; direct T60 edits must not be undone.
bool Eligible(const Document &baseline, const Document &edited);
Cells RenderMeasure(const Document &, uint32_t seed, unsigned rate,
                    const Cancel &cancel = {});
} // namespace drumfoundry::decay_hold
