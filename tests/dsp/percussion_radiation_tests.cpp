#include "percussion_test_support.hpp"

#include "tfdsp/percussion/biquad.hpp"
#include "tfdsp/percussion/biquad_design.hpp"
#include "tfdsp/percussion/live_output_eq.hpp"
#include "tfdsp/percussion/radiation_filter.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace {

using percussion_test::Check;
using percussion_test::CheckNear;

template <typename Filter>
double ToneGain(Filter &filter, const float frequencyHz, const float sampleRate) {
  double inputEnergy = 0.0;
  double outputEnergy = 0.0;
  for (std::size_t sample = 0; sample < 65536; ++sample) {
    const float input = percussion_test::Sine(sample, frequencyHz, sampleRate);
    const float output = filter.Process(input);
    if (sample < 4096)
      continue;
    inputEnergy += input * input;
    outputEnergy += output * output;
  }
  return std::sqrt(outputEnergy / inputEnergy);
}

void TestBiquadResponses() {
  for (const float sampleRate : {44100.f, 48000.f, 96000.f, 192000.f}) {
    for (float q : {.1f, .7f, 4.f, 20.f}) {
      tfdsp::percussion::Biquad peak;
      peak.SetCoefficients(tfdsp::percussion::biquad_design::Peaking(
          5000.f, q, 9.f, sampleRate));
      CheckNear(ToneGain(peak, 5000.f, sampleRate),
                std::pow(10.0, 9.0 / 20.0), .006,
                "peaking biquad reaches its requested centre gain");
    }

    tfdsp::percussion::Biquad lowpass;
    lowpass.SetCoefficients(tfdsp::percussion::biquad_design::Lowpass(
        6000.f, .70710678f, sampleRate));
    Check(ToneGain(lowpass, 1000.f, sampleRate) > .99,
          "radiation low-pass preserves its lower passband");
    Check(ToneGain(lowpass, .4f * sampleRate, sampleRate) < .2,
          "radiation low-pass rejects near-Nyquist content");
  }
}

// Low cutoff TDF-II cancellation must not turn a linear filter into a
// level-dependent processor, including at high host sample rates.
void TestLowCutoffLinearity() {
  for (const float sampleRate : {44100.f, 192000.f}) {
    tfdsp::percussion::Biquad full, scaled;
    const auto coefficients = tfdsp::percussion::biquad_design::Highpass(
        5.f, .70710678f, sampleRate);
    full.SetCoefficients(coefficients);
    scaled.SetCoefficients(coefficients);
    double maximumError = 0.;
    for (std::size_t sample = 0; sample < 192000; ++sample) {
      const float input = .6f * percussion_test::Sine(sample, 27.f, sampleRate)
          + .2f * percussion_test::Sine(sample, 117.f, sampleRate);
      const float output = full.Process(input);
      const float attenuated = scaled.Process(.3f * input);
      maximumError = std::max(maximumError,
          std::abs(static_cast<double>(attenuated) - .3 * output));
    }
    Check(maximumError < 1.e-6,
          "5 Hz high-pass preserves gain linearity without cancellation drift");
  }
}

void TestRadiationChain() {
  tfdsp::percussion::RadiationFilter filter;
  tfdsp::percussion::RadiationFilterParameters parameters;
  parameters.lowCutHz = 300.f;
  parameters.colourFrequencyHz = 4500.f;
  parameters.colourGainDb = 6.f;
  parameters.highCutHz = 12000.f;
  filter.Prepare(48000.f, parameters);
  const double low = ToneGain(filter, 60.f, 48000.f);
  filter.Reset();
  const double colour = ToneGain(filter, 4500.f, 48000.f);
  filter.Reset();
  const double high = ToneGain(filter, 20000.f, 48000.f);
  Check(colour > 3.0 * low && colour > 3.0 * high,
        "radiation chain separates body colour from spectral extremes");
  Check(std::isfinite(filter.Process(std::numeric_limits<float>::infinity())),
        "radiation chain sanitizes non-finite input");
}

void TestLiveEq() {
  using namespace tfdsp::percussion;
  for (float rate : {32000.f, 44100.f, 48000.f, 96000.f, 192000.f}) {
    LiveOutputEq live;
    RadiationFilterParameters p;
    live.Prepare(rate, p, false);
    for (unsigned i = 0; i < unsigned(rate); ++i) {
      if (i % 127 == 0) {
        const bool up = (i / 127) % 2;
        p.lowCutHz = up ? 1000 : 5;
        p.highCutHz = up ? 22000 : 500;
        p.colourFrequencyHz = up ? 20000 : 40;
        p.colourGainDb = up ? 24 : -24;
        p.colourQ = up ? 20.f : .1f;
        live.SetTarget(p, (i / 127) % 3 != 0);
      }
      const float input = .05f * percussion_test::Sine(i, 83.f, rate) +
                          .05f * percussion_test::Sine(i, 7000.f, rate);
      const float out = live.Process(input);
      if (!std::isfinite(out) || std::abs(out) > 8.f) {
        Check(false, "Rapid full-range EQ/bypass edits remain bounded");
        break;
      }
    }
  }
}
} // namespace

int main() {
  TestBiquadResponses();
  TestLowCutoffLinearity();
  TestRadiationChain();
  TestLiveEq();
  if (percussion_test::failures == 0)
    std::cout << "All percussion radiation tests passed\n";
  return percussion_test::failures == 0 ? 0 : 1;
}
