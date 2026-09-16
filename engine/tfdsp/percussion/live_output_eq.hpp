#pragma once
#include "live_gain.hpp"
#include "radiation_filter.hpp"

namespace tfdsp::percussion {
// Final radiation EQ with live coefficient and bypass transitions. The filter
// stays warm while bypassed, so enabling it does not start an empty filter.
class LiveOutputEq {
public:
  void Prepare(float rate, const RadiationFilterParameters &p, bool enabled) {
    rate_ = rate;
    parameters_ = p;
    filter_.Prepare(rate, p);
    wet_.Reset(enabled ? 1.f : 0.f);
  }
  void SetTarget(const RadiationFilterParameters &p, bool enabled) noexcept {
    if (p.lowCutHz != parameters_.lowCutHz ||
        p.lowCutQ != parameters_.lowCutQ ||
        p.highCutHz != parameters_.highCutHz ||
        p.highCutQ != parameters_.highCutQ ||
        p.colourFrequencyHz != parameters_.colourFrequencyHz ||
        p.colourGainDb != parameters_.colourGainDb ||
        p.colourQ != parameters_.colourQ ||
        p.outputGain != parameters_.outputGain) {
      filter_.SetTargetParameters(p);
      parameters_ = p;
    }
    wet_.Target(enabled ? 1.f : 0.f, rate_);
  }
  void Reset() noexcept { filter_.Reset(); }
  float Process(float input) noexcept {
    const float filtered = filter_.Process(input);
    const float wet = wet_.Next();
    if (wet == 0.f)
      return input;
    if (wet == 1.f)
      return filtered;
    return input + wet * (filtered - input);
  }

private:
  RadiationFilter filter_;
  RadiationFilterParameters parameters_{};
  LiveGain wet_;
  float rate_{48000.f};
};
} // namespace tfdsp::percussion
