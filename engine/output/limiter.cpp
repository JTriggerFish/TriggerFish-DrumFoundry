#include "limiter.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace drumfoundry::output {
namespace {
double Db(double amplitude) noexcept {
  return 20 * std::log10(std::max(amplitude, 1e-8));
}
double ReductionDb(double gain) noexcept {
  return -20 * std::log10(std::max(gain, std::numeric_limits<double>::min()));
}
} // namespace

void Limiter::Prepare(double sampleRate, std::size_t channels, bool enabled) {
  if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 384000 ||
      channels < 1 || channels > 2)
    throw std::invalid_argument("Limiter requires 8–384 kHz, mono or stereo");
  sampleRate_ = sampleRate;
  channels_ = channels;
  enabled_ = enabled;
  delaySamples_ =
      static_cast<std::size_t>(std::lround(LookaheadSeconds * sampleRate));
  const auto radius = std::min<std::size_t>(32, delaySamples_ / 2);
  // Detector latency plus total FIR support equals audio delay, not two delays.
  const auto support = delaySamples_ - radius;
  smoothing_[0].Prepare(support / 2 + 1);
  smoothing_[1].Prepare(support - support / 2 + 1);
  hold_.Prepare(delaySamples_ +
                static_cast<std::size_t>(std::ceil(HoldSeconds * sampleRate)) +
                1);
  for (std::size_t c = 0; c < channels; ++c) {
    detectors_[c].Prepare(radius);
    delay_[c].resize(delaySamples_);
  }
  releaseStep_ = -std::expm1(-1 / (ReleaseSeconds * sampleRate));
  protectedCeiling_ = std::pow(10., (CeilingDb - ReconstructionReserveDb) / 20);
  Reset();
}

void Limiter::Reset() noexcept {
  for (auto &delay : delay_)
    std::fill(delay.begin(), delay.end(), 0.f);
  for (auto &detector : detectors_)
    detector.Reset();
  for (auto &average : smoothing_)
    average.Reset();
  hold_.Reset();
  write_ = 0;
  releaseGain_ = gain_ = 1;
  ClearMeters();
}

void Limiter::ClearMeters() noexcept {
  minimumGain_ = gain_;
  inputPeak_ = outputPeak_ = 0;
  invalidInput_ = false;
}

double Limiter::FollowGain(double peak) noexcept {
  const double held = hold_.Process(peak);
  const double target =
      held > protectedCeiling_ ? protectedCeiling_ / held : 1.;
  releaseGain_ =
      std::min(target, releaseGain_ + releaseStep_ * (target - releaseGain_));
  return smoothing_[1].Process(smoothing_[0].Process(releaseGain_));
}

std::array<float, 2>
Limiter::ProcessFrame(std::array<float, 2> input) noexcept {
  if (channels_ == 0)
    return {}; // Unprepared objects are silent.
  std::array<float, 2> delayed{};
  double peak = 0;
  for (std::size_t c = 0; c < channels_; ++c) {
    if (!std::isfinite(input[c])) {
      input[c] = 0;
      invalidInput_ = true;
    }
    peak = std::max(peak, enabled_ ? detectors_[c].Process(input[c])
                                   : std::abs(double(input[c])));
    if (enabled_) {
      delayed[c] = delay_[c][write_];
      delay_[c][write_] = input[c];
    } else
      delayed[c] = input[c];
  }
  gain_ = enabled_ ? FollowGain(peak) : 1.;
  double outPeak = 0;
  for (std::size_t c = 0; c < channels_; ++c) {
    delayed[c] = static_cast<float>(delayed[c] * gain_);
    outPeak = std::max(outPeak, std::abs(double(delayed[c])));
  }
  write_ = (write_ + 1) % delaySamples_;
  Observe(peak, outPeak);
  return delayed;
}

bool Limiter::Process(float *const *channels, std::size_t frames) noexcept {
  if (!channels_ || (frames && !channels))
    return false;
  for (std::size_t c = 0; frames && c < channels_; ++c)
    if (!channels[c])
      return false;
  for (std::size_t i = 0; i < frames; ++i) {
    const auto output =
        ProcessFrame({channels[0][i], channels_ == 2 ? channels[1][i] : 0.f});
    for (std::size_t c = 0; c < channels_; ++c)
      channels[c][i] = output[c];
  }
  return true;
}

void Limiter::Observe(double inputPeak, double outputPeak) noexcept {
  minimumGain_ = std::min(minimumGain_, gain_);
  inputPeak_ = std::max(inputPeak_, inputPeak);
  outputPeak_ = std::max(outputPeak_, outputPeak);
}

LimiterStatus Limiter::Status() const noexcept {
  const auto latency = enabled_ ? delaySamples_ : 0;
  return {enabled_,
          latency,
          1000. * latency / sampleRate_,
          ReductionDb(gain_),
          ReductionDb(minimumGain_),
          Db(inputPeak_),
          Db(outputPeak_),
          invalidInput_};
}
} // namespace drumfoundry::output
