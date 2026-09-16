#pragma once

#include <algorithm>
#include <cmath>

namespace tfdsp::percussion {
// Unit-mass reduced contact motion. The nonnegative potential is stored directly
// (g*h), so changing return acceleration changes travel, not stored energy.
// This is a perceptual rattle coordinate, NOT a simulated second cymbal shell.
struct RimRattleState {
  double velocity{}, potential{};

  double Energy() const noexcept { return .5 * velocity * velocity + potential; }
  bool Moving() const noexcept { return velocity != 0 || potential != 0; }

  // Exact ballistic flight up to the next impact. An impact is resolved by the
  // caller against modal velocity; no independent strike/noise is generated.
  bool Advance(double acceleration, double dt) noexcept {
    if (!Moving() || acceleration <= 0) return false;
    const double next = velocity - acceleration * dt;
    const double energy = Energy();
    const double nextPotential = energy - .5 * next * next;
    if (next < 0 && nextPotential <= 0) {
      velocity = -std::sqrt(2 * energy);
      potential = 0;
      return true;
    }
    velocity = next;
    potential = std::max(0.0, nextPotential);
    return false;
  }

  // Sub-sample bounces enter the resting-contact regime. Account for their
  // remaining energy as loss, rather than letting unresolved chatter alias.
  double Capture(double acceleration, double dt) noexcept {
    if (potential > 0 || velocity > acceleration * dt) return 0;
    const double loss = Energy();
    velocity = potential = 0;
    return loss;
  }
};
} // namespace tfdsp::percussion
