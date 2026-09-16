#pragma once

#include "enveloped_noise_burst.hpp"
#include <array>

namespace tfdsp::percussion {

// Direct radiation only: no body port, energy feedback or force pulse. Reuse
// the contact noise primitive, with independent envelopes for overlapping taps.
// Eight contacts cover dense rolls; pathological overflow drops the new accent
// rather than cutting a sounding envelope. Body strikes are never dropped.
class ObservedNoiseStrike {
public:
  void Prepare(float rate) {
    for (auto &voice : voices_)
      voice.Prepare(rate);
    Reset();
  }
  void Reset() noexcept {
    for (auto &voice : voices_)
      voice.Reset();
    next_ = 0;
    active_ = false;
  }
  void Trigger(float strength, float t60, float colour,
               std::uint32_t seed) noexcept {
    std::size_t slot = next_;
    for (std::size_t i = 0; i < voices_.size(); ++i) {
      slot = (next_ + i) % voices_.size();
      if (!voices_[slot].Active())
        break;
    }
    if (voices_[slot].Active())
      return;
    EnvelopedNoiseBurstParameters p;
    p.attackSeconds = .0002f; // Smooth onset; not an extra playback delay.
    p.holdSeconds = 0.f;
    // The shared primitive stops at -80 dB. Its public control here is T60.
    p.decaySeconds = std::clamp(FiniteNormalOrZero(t60), .002f, .12f) * (4.f / 3.f);
    p.amplitude = std::clamp(FiniteNormalOrZero(strength), 0.f, 1.f);
    p.tiltDb = colour;
    p.tiltPivotHz = 4200.f;
    p.seed = seed ^ 0x5354524bu;
    voices_[slot].Trigger(p);
    next_ = (slot + 1) % voices_.size();
    active_ = true;
  }
  float Process() noexcept {
    if (!active_)
      return 0.f;
    float result = 0.f;
    active_ = false;
    for (auto &voice : voices_)
      if (voice.Active()) {
        result += voice.Process();
        active_ = true;
      }
    return FiniteNormalOrZero(result);
  }

private:
  std::array<EnvelopedNoiseBurst, 8> voices_{};
  std::size_t next_{};
  bool active_{};
};
} // namespace tfdsp::percussion
