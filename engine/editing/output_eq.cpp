#include "output_eq.hpp"
#include "tfdsp/percussion/output_eq_parameters.hpp"
#include <algorithm>
#include <complex>
namespace drumfoundry::editing {
bool HasOutputEq(const Document &document) {
  const auto &p = document.Parameters();
  return std::any_of(p.begin(), p.end(), [](const auto &p) {
    return p.key == "output_eq_enabled";
  });
}
tfdsp::percussion::RadiationFilterParameters
OutputEqSettings(const Document &d) {
  return tfdsp::percussion::SimpleOutputEqParameters(
      float(d.Value("output_low_cut")),
      float(d.Value("output_colour_frequency")),
      float(d.Value("output_colour_gain")), float(d.Value("output_high_cut")),
      float(d.Value("output_colour_q")));
}
OutputEqResponse::OutputEqResponse(
    const tfdsp::percussion::RadiationFilterParameters &p, double rate)
    : rate_(rate) {
  if (!std::isfinite(rate) || rate < 8000 || rate > 384000)
    throw std::invalid_argument("Invalid EQ response sample rate");
  using namespace tfdsp::percussion::biquad_design;
  stages_ = {
      Highpass(p.lowCutHz, p.lowCutQ, float(rate)),
      Peaking(p.colourFrequencyHz, p.colourQ, p.colourGainDb, float(rate)),
      Lowpass(p.highCutHz, p.highCutQ, float(rate))};
}
std::array<double, 3> OutputEqResponse::At(double hz) const {
  if (!std::isfinite(hz) || hz < 0 || hz >= rate_ / 2)
    throw std::invalid_argument("Invalid EQ response frequency");
  const auto z = std::polar(1., -6.283185307179586 * hz / rate_);
  std::array<double, 3> result{};
  for (unsigned i = 0; i < stages_.size(); ++i) {
    const auto &s = stages_[i];
    const auto response =
        (s.b0 + s.b1 * z + s.b2 * z * z) / (1. + s.a1 * z + s.a2 * z * z);
    result[i] = 20 * std::log10(std::max(1e-15, std::abs(response)));
  }
  return result;
}
} // namespace drumfoundry::editing
