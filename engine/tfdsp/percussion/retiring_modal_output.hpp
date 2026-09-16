#pragma once
#include "modal_constraint.hpp"
#include "tfdsp/finite_audio.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace tfdsp::percussion {
// Observation-only continuation of removed oscillators. Their energy has
// already been remapped into the body; these samples never drive it again.
// No new excitation, stochastic motion, coupling, allocation or second voice.
template <std::size_t Capacity> class RetiringModalOutput {
public:
  void Reset() noexcept { count_ = remaining_ = 0; }
  bool Active() const noexcept { return remaining_ != 0; }

  void Add(float real, float imaginary, float cosine, float sine,
           float radius, float gain, unsigned band) noexcept {
    // Caller supplies a subset of one previous layout (at most Capacity).
    if (gain == 0.f || (real == 0.f && imaginary == 0.f))
      return;
    // The centres of the +/- blur rotations have the nominal angle but not
    // unit length. Normalize once, then continue at that angle during fadeout.
    const float norm = std::hypot(cosine, sine);
    modes_[count_++] = {real, imaginary, cosine / norm, sine / norm,
                       radius, gain, band};
  }
  void Start(unsigned samples) noexcept {
    length_ = std::max(1u, samples);
    remaining_ = count_ ? length_ : 0;
  }
  float Process(ModalDampingGains damping) noexcept {
    if (!remaining_)
      return 0.f;
    const float fade = float(--remaining_) / float(length_);
    const std::array<float, 3> bands{damping.low, damping.middle, damping.high};
    float output = 0.f;
    for (std::size_t i = 0; i < count_; ++i) {
      auto &m = modes_[i];
      const float radius = m.radius * damping.broadband * bands[m.band];
      const float real = m.cosine * m.real - m.sine * m.imaginary;
      m.imaginary = tfdsp::FiniteNormalOrZero(
          radius * (m.sine * m.real + m.cosine * m.imaginary));
      m.real = tfdsp::FiniteNormalOrZero(radius * real);
      output += m.gain * m.real;
    }
    return tfdsp::FiniteNormalOrZero(fade * output);
  }

private:
  struct Mode {
    float real{}, imaginary{}, cosine{}, sine{}, radius{}, gain{};
    unsigned band{};
  };
  std::array<Mode, Capacity> modes_{};
  std::size_t count_{};
  unsigned remaining_{}, length_{};
};
} // namespace tfdsp::percussion
