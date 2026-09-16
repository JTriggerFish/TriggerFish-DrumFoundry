#include "wire_rack.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tfdsp::percussion {
namespace {
float Safe(float value, float fallback, float low, float high) noexcept {
  return std::clamp(std::isfinite(value) ? value : fallback, low, high);
}
} // namespace

WireRackLiveParameters PrepareWireRackLiveParameters(
    float sampleRate, const WireRackParameters &source) noexcept {
  WireRackLiveParameters result;
  constexpr float TwoPi = 6.28318530717958647692f;
  const float motionHz = Safe(source.motionHighpassHz, 140.f, 5.f, .45f * sampleRate);
  result.motionCoefficient = 1.f - std::exp(-TwoPi * motionHz / sampleRate);
  const float attack = Safe(source.attackSeconds, .002f, .0001f, .08f);
  const float release = Safe(source.releaseSeconds, .018f, .0005f, 1.f);
  result.attackCoefficient = std::exp(-1.f / (attack * sampleRate));
  result.releaseCoefficient = std::exp(-1.f / (release * sampleRate));
  result.sensitivity = Safe(source.sensitivity, 1.125f, 0.f, 8.f);
  result.threshold = Safe(source.threshold, .004f, 0.f, 1.f);
  result.noiseTiltDb = 16.f * (Safe(source.brightness, .62f, 0.f, 1.f) - .5f);
  result.noiseMix = Safe(source.noiseMix, .6f, 0.f, 2.f);
  result.modalMix = Safe(source.modalMix, .75f, 0.f, 2.f);
  return result;
}

WireRackPreparedParameters PrepareWireRackParameters(
    float sampleRate, const WireRackParameters &source) {
  if (!std::isfinite(sampleRate) || sampleRate < 1.f)
    throw std::invalid_argument("wire-rack sample rate must be positive");
  WireRackPreparedParameters result;
  result.sampleRate = sampleRate;
  const float low = Safe(source.minimumFrequencyHz, 900.f, 40.f, .4f * sampleRate);
  const float high = Safe(source.maximumFrequencyHz, 15500.f, low + 1.f, .48f * sampleRate);
  const float density = Safe(source.density, .8f, 0.f, 1.f);
  result.activeModeCount = std::clamp<std::size_t>(
      static_cast<std::size_t>(std::lround(8.f + density * (WireRackModeCount - 8.f))),
      8, WireRackModeCount);
  const float decay = Safe(source.decaySeconds, .16f, .008f, 3.f);
  const float decayTilt = Safe(source.decayTilt, .7f, -1.f, 1.f);
  DeterministicRandom random;
  random.Seed(source.seed);
  float outputNormSquared = 0.f;
  constexpr float TwoPi = 6.28318530717958647692f;
  for (std::size_t mode = 0; mode < result.activeModeCount; ++mode) {
    const float position = (static_cast<float>(mode) + .5f + .38f * random.Bipolar()) /
                           static_cast<float>(result.activeModeCount);
    const float frequency = low * std::pow(high / low, std::clamp(position, 0.f, 1.f));
    const float angle = TwoPi * frequency / sampleRate;
    const float modeDecay = decay * std::exp2(-2.f * decayTilt * std::clamp(position, 0.f, 1.f));
    const float phase = 3.14159265358979323846f * random.Bipolar();
    const float gain = std::exp2(-.35f * position) * (.75f + .5f * random.Uniform());
    result.cosine[mode] = std::cos(angle);
    result.sine[mode] = std::sin(angle);
    result.radius[mode] = std::exp(std::log(.001f) / (modeDecay * sampleRate));
    result.inputPhaseCosine[mode] = std::cos(phase);
    result.inputPhaseSine[mode] = std::sin(phase);
    result.modeOutputGain[mode] = gain;
    outputNormSquared += gain * gain;
  }
  const float outputScale = 1.f / std::sqrt(std::max(outputNormSquared, 1.e-12f));
  for (std::size_t mode = 0; mode < result.activeModeCount; ++mode)
    result.modeOutputGain[mode] *= outputScale;
  result.controls = PrepareWireRackLiveParameters(sampleRate, source);
  result.seed = source.seed;
  return result;
}
} // namespace tfdsp::percussion
