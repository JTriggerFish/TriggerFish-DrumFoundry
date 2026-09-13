#pragma once
#include <array>
#include <functional>
#include <vector>
namespace drumfoundry::decay_hold {
using Cells = std::array<double, 264>;
using Cancel = std::function<bool()>;
// Fixed comparison coordinates, independent of display FFT/zoom/colour.
Cells Measure(const std::vector<float> &, unsigned sampleRate,
              const Cancel &cancel = {});
struct Errors {
  std::vector<double> late, front;
};
class Targets {
public:
  Targets(const Cells &baseline, const Cells &edited);
  Errors Compare(const Cells &) const;

private:
  Cells baseline_, edited_;
  std::vector<unsigned> late_, front_;
  double floor_{};
};
double Rms(const std::vector<double> &);
void CheckCancelled(const Cancel &);
} // namespace drumfoundry::decay_hold
