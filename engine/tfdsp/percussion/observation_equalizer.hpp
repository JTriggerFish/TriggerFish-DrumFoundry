#pragma once

#include "biquad.hpp"
#include "biquad_design.hpp"
#include "live_output_eq.hpp"
#include "radiation_filter.hpp"
#include "tfdsp/finite_audio.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace tfdsp::percussion {

enum class ObservationEqualizerMode : int { Bypass, Radiation, Multiband };

struct ObservationEqualizerBand {
  float frequencyHz{1000.f};
  float gainDb{};
  float q{.7f};
};

struct ObservationEqualizerParameters {
  ObservationEqualizerMode mode{ObservationEqualizerMode::Radiation};
  RadiationFilterParameters radiation{};
  float lowCutHz{20.f};
  float highCutHz{19000.f};
  std::array<ObservationEqualizerBand, 4> bands{{
      {90.f, 0.f, .7f}, {350.f, 0.f, .7f},
      {1800.f, 0.f, .7f}, {7500.f, 0.f, .7f}}};
  float outputGain{1.f};
};

// Swappable output presentation. It is deliberately outside every resonator
// loop so that EQ changes cannot create or remove stored body energy.
class ObservationEqualizer {
public:
  void Prepare(const float sampleRate,
               const ObservationEqualizerParameters &parameters) {
    if (!std::isfinite(sampleRate) || sampleRate < 1.f)
      throw std::invalid_argument("observation EQ sample rate must be positive");
    sampleRate_ = sampleRate;
    SetStaticParameters(parameters);
    Reset();
  }

  void Reset() noexcept {
    liveRadiation_.Reset();
    highpass_.Reset();
    lowpass_.Reset();
    for (auto &band : bands_) band.Reset();
  }

  void SetStaticParameters(
      const ObservationEqualizerParameters &parameters) noexcept {
    mode_ = parameters.mode;
    liveRadiation_.Prepare(sampleRate_, parameters.radiation,
                           mode_ == ObservationEqualizerMode::Radiation);
    highpass_.SetCoefficients(biquad_design::Highpass(
        parameters.lowCutHz, .707f, sampleRate_));
    lowpass_.SetCoefficients(biquad_design::Lowpass(
        parameters.highCutHz, .707f, sampleRate_));
    for (std::size_t index = 0; index < bands_.size(); ++index) {
      const auto &source = parameters.bands[index];
      bands_[index].SetCoefficients(biquad_design::Peaking(
          source.frequencyHz, source.q, source.gainDb, sampleRate_));
    }
    outputGain_ = std::clamp(
        tfdsp::FiniteNormalOrZero(parameters.outputGain), 0.f, 16.f);
  }

  float Process(float input) noexcept {
    input = tfdsp::FiniteNormalOrZero(input);
    if (mode_ != ObservationEqualizerMode::Multiband)
      return tfdsp::FiniteNormalOrZero(outputGain_ *
                                       liveRadiation_.Process(input));
    if (mode_ == ObservationEqualizerMode::Multiband) {
      input = highpass_.Process(input);
      for (auto &band : bands_) input = band.Process(input);
      input = lowpass_.Process(input);
    }
    return tfdsp::FiniteNormalOrZero(outputGain_ * input);
  }

  // The native control surface exposes only radiation/bypass, not multiband.
  void SetLiveParameters(const ObservationEqualizerParameters &p) noexcept {
    mode_ = p.mode;
    liveRadiation_.SetTarget(p.radiation,
                             p.mode == ObservationEqualizerMode::Radiation);
    outputGain_ = p.outputGain;
  }

private:
  LiveOutputEq liveRadiation_{};
  Biquad highpass_{};
  Biquad lowpass_{};
  std::array<Biquad, 4> bands_{};
  ObservationEqualizerMode mode_{ObservationEqualizerMode::Radiation};
  float sampleRate_{48000.f};
  float outputGain_{1.f};
};

} // namespace tfdsp::percussion
