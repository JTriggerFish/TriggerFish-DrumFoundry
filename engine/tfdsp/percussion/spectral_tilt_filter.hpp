#pragma once

#include "tfdsp/finite_audio.hpp"
#include "live_gain.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tfdsp::percussion {

// Complementary one-pole shelf tilt. Zero dB is an exact wire, negative tilt
// darkens the source, and positive tilt emphasizes its upper band.
class SpectralTiltFilter {
public:
  void Prepare(const float sampleRate) {
    if (!std::isfinite(sampleRate) || sampleRate < 1.f)
      throw std::invalid_argument("tilt-filter sample rate must be positive");
    sampleRate_ = sampleRate;
    SetTilt(0.f, 3000.f);
    Reset();
  }

  void Reset() noexcept { lowState_ = 0.f; }

  void SetTilt(float tiltDb, float pivotHz) noexcept {
    Configure(tiltDb, pivotHz, false);
  }

  // Fixed-pivot live colour changes retain filter memory and smooth shelf gains.
  void SetLiveTilt(float tiltDb, float pivotHz) noexcept {
    Configure(tiltDb, pivotHz, true);
  }

  float Process(const float input) noexcept {
    const float safeInput = tfdsp::FiniteNormalOrZero(input);
    lowState_ += coefficient_ * (safeInput - lowState_);
    lowState_ = tfdsp::FiniteNormalOrZero(lowState_);
    const float high = safeInput - lowState_;
    return tfdsp::FiniteNormalOrZero(
        lowGain_.Next() * lowState_ + highGain_.Next() * high);
  }

private:
  void Configure(float tiltDb, float pivotHz, bool live) noexcept {
    tiltDb = std::clamp(std::isfinite(tiltDb) ? tiltDb : 0.f, -24.f, 24.f);
    pivotHz = std::clamp(std::isfinite(pivotHz) ? pivotHz : 3000.f,
                         20.f, .45f * sampleRate_);
    coefficient_ = 1.f - std::exp(-6.283185307179586f * pivotHz / sampleRate_);
    const float low = std::pow(10.f, -tiltDb / 40.f);
    const float high = std::pow(10.f, tiltDb / 40.f);
    if (live) {
      lowGain_.Target(low, sampleRate_);
      highGain_.Target(high, sampleRate_);
    } else {
      lowGain_.Reset(low);
      highGain_.Reset(high);
    }
  }
  float sampleRate_{48000.f};
  float coefficient_{};
  float lowState_{};
  LiveGain lowGain_, highGain_;
};

} // namespace tfdsp::percussion
