#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace tfdsp::percussion {
// Cycle-averaged friction-like loss, not waveform clipping or an output gate.
// Radially shrink each quadrature state: A' = max(0, A - c * dt * |w|).
// Normalized modal drive weights w make subdivision into equal-energy modes
// preserve the decay law. Observation gains and current strike level do not
// change the loss. Ordinary exponential damping remains in the modal poles.
// This is a first-order split of dA/dt = -lambda*A - c*|w|. No peak tracker,
// elapsed-hit timer or dead-mode latch: incoming energy can always revive a mode.
template <std::size_t N>
void ApplyModalFrictionLoss(std::array<float, N> &real,
                            std::array<float, N> &imaginary,
                            const std::array<float, N> &weights,
                            std::size_t count, float decrement) noexcept {
  if (!(decrement > 0.f))
    return;
  for (std::size_t i = 0; i < count; ++i) {
    const float drop = decrement * std::abs(weights[i]);
    const float energy = real[i] * real[i] + imaginary[i] * imaginary[i];
    if (energy <= drop * drop) {
      real[i] = imaginary[i] = 0.f;
    } else {
      const float gain = 1.f - drop / std::sqrt(energy);
      real[i] *= gain;
      imaginary[i] *= gain;
    }
  }
}
} // namespace tfdsp::percussion
