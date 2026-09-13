#pragma once
#include "document.hpp"
#include "tfdsp/percussion/radiation_filter.hpp"
#include <array>
namespace drumfoundry::editing {
bool HasOutputEq(const Document &);
tfdsp::percussion::RadiationFilterParameters OutputEqSettings(const Document &);
// dB contribution of the exact runtime high-pass, colour peak and low-pass.
class OutputEqResponse {
public:
  OutputEqResponse(const tfdsp::percussion::RadiationFilterParameters &,
                   double rate);
  std::array<double, 3> At(double frequency) const;

private:
  std::array<tfdsp::percussion::BiquadCoefficients, 3> stages_;
  double rate_;
};
} // namespace drumfoundry::editing
