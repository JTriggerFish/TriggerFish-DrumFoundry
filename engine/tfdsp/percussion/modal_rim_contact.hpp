#pragma once

#include "live_gain.hpp"
#include "rim_rattle_state.hpp"
#include "../smooth_random_modulator.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace tfdsp::percussion {

struct ModalRimContactParameters {
  bool enabled{};
  float openness{1.f};
  float clearance{.001f}; // displacement in normalized velocity-milliseconds
  float loss{.25f};       // fraction of relative kinetic energy lost per impact
  float pedalStrength{1.f}; // effective closing-velocity coupling, v*milliseconds
  float motion{};          // correlated movement between rim participation patterns
  float settling{};        // slow to fast return acceleration; zero is NOT bypass
};

// Reduced relative-rim coordinates, not a second synthesized cymbal. Each
// impulse projects onto the SAME modal velocities that produce the audio.
// Fixed-boundary impacts are passive; a moving pedal supplies explicit work.
template <std::size_t Count> class ModalRimContact {
public:
  using Values = std::array<float, Count>;
  static constexpr std::size_t PortCount = 4;

  void Prepare(float rate, const Values &frequencies, const Values &input,
               const std::array<std::uint32_t, Count> &packets,
               std::size_t active) noexcept {
    rate_ = rate;
    active_ = std::min(active, Count);
    for (std::size_t i = 0; i < active_; ++i)
      inverseOmega_[i] = 1.f / (6.28318530718f * std::max(1.f, frequencies[i]));
    for (std::size_t p = 0; p < PortCount; ++p) {
      double norm = 0;
      for (std::size_t i = 0; i < active_; ++i) {
        // Stable packet geometry: subdivision changes neither relative mass
        // nor spatial participation. Satellite modes inherit their handle.
        const std::uint32_t hash = (packets[i] ^ 0x9e3779b9u) * 2654435761u;
        const double angle = (double(hash) / 4294967296.0) * 6.28318530718;
        const float w = std::abs(input[i]) * float(std::cos((p + 1) * angle));
        weights_[p][i] = w;
        norm += double(w) * w;
      }
      const float scale = norm > 1.e-30 ? float(1.0 / std::sqrt(norm)) : 0.f;
      double mass = 0;
      for (std::size_t i = 0; i < active_; ++i) {
        weights_[p][i] *= scale;
        mass += double(weights_[p][i]) * weights_[p][i];
      }
      inverseMass_[p] = float(mass);
      motion_[p].Prepare(rate / 32, 2.f + .3f * p, 73519u + 7919u * p);
    }
    Reset();
  }

  void SetParameters(ModalRimContactParameters next, bool immediate = false) noexcept {
    next.openness = Safe(next.openness, 0.f, 1.f);
    next.clearance = Safe(next.clearance, 0.f, 2.f);
    next.loss = Safe(next.loss, 0.f, 1.f);
    next.pedalStrength = Safe(next.pedalStrength, 0.f, 4.f);
    next.motion = Safe(next.motion, 0.f, 1.f);
    next.settling = Safe(next.settling, 0.f, 1.f);
    // Enabling/configuring contact is not pedal work. Only an openness gesture
    // can inject energy; loading a closed patch must be silent.
    if (immediate || next.enabled != parameters_.enabled) {
      pedal_.Reset(next.openness);
      settling_.Reset(next.settling);
    } else {
      pedal_.Target(next.openness, rate_);
      settling_.Target(next.settling, rate_);
    }
    parameters_ = next;
    if (!next.enabled) rattle_ = {};
    restitution_ = std::sqrt(1.f - next.loss);
  }

  void Reset() noexcept {
    pedal_.Reset(parameters_.openness);
    settling_.Reset(parameters_.settling);
    work_ = dissipated_ = 0;
    collisions_ = 0;
    rattle_ = {};
    for (auto &motion : motion_) motion.Reset();
    motionSample_ = 0;
    movingWeights_ = weights_;
    gapScale_.fill(1.f);
  }
  void RetainState(const ModalRimContact &old) noexcept {
    parameters_ = old.parameters_;
    restitution_ = old.restitution_;
    pedal_ = old.pedal_;
    settling_ = old.settling_;
    rattle_ = old.rattle_;
    motion_ = old.motion_;
    motionSample_ = 0;
  }
  const ModalRimContactParameters &Parameters() const noexcept { return parameters_; }
  // Pitch/tension changes alter displacement conversion, not stored energy.
  void SetFrequency(std::size_t i, float hz) noexcept {
    if (i < active_)
      inverseOmega_[i] = 1.f / (6.28318530718f * std::max(1.f, hz));
  }

  void Process(Values &velocity, const Values &scaledDisplacement) noexcept {
    work_ = dissipated_ = 0;
    collisions_ = 0;
    const float previous = pedal_.Current();
    const float position = pedal_.Next();
    settling_.Next();
    if (!parameters_.enabled) return;
    UpdateMotion();
    // 0.001 is a units conversion (velocity-milliseconds -> displacement),
    // not a gain. Clearance edits themselves do not create wall velocity.
    // Cubic pedal travel gives the small grazing-contact gaps useful control
    // space. It is the same mapping for UI, MIDI and offline rendering.
    const float travel = position * position * position;
    const float oldTravel = previous * previous * previous;
    const float gap = .001f * parameters_.clearance * travel;
    const float wallVelocity = .001f * parameters_.clearance *
        (travel - oldTravel) * rate_;
    // The foot supplies external impact energy independently of micrometre-
    // scale vibrating-rim clearance. This explicit reduced actuator port is
    // not a geometric shell solver, nor an independent audio/noise source.
    const float closingRate = std::max(previous - position, 0.f) * rate_;
    // Four full pedal strokes/second is the response knee. Quadratic near rest,
    // linear for fast closure: slow sustained movement must not bow the body.
    // This is the explicit foot actuator response, NOT strike velocity mapping.
    const float footVelocity = -.001f * parameters_.pedalStrength *
        closingRate * closingRate / (4.f + closingRate);
    for (std::size_t p = 0; p < PortCount; ++p)
      Impact(p, parameters_.motion > 0 ? gap * gapScale_[p] : gap,
             wallVelocity + footVelocity, wallVelocity, velocity, scaledDisplacement);
  }

  // Physical energy convention: 1/2 sum(v^2 + (omega*q)^2).
  double LastWork() const noexcept { return work_; }
  double LastDissipation() const noexcept { return dissipated_; }
  unsigned LastCollisions() const noexcept { return collisions_; }
  double StoredEnergy() const noexcept {
    double result = 0;
    for (const auto &state : rattle_) result += state.Energy();
    return result;
  }

private:
  double ReturnAcceleration() const noexcept {
    // Explicit control range: 0.01..1 normalized acceleration. Zero means slow
    // settling, not an immobile boundary or an infinitely suspended rattle.
    // Keeping the mass/coupling independent avoids closing the hat at the left
    // endpoint. The square gives the slower region more editing space.
    const double s = settling_.Current();
    return .01 + .99 * s * s;
  }
  static float Safe(float v, float lo, float hi) noexcept {
    return std::isfinite(v) ? std::clamp(v, lo, hi) : lo;
  }
  void Impact(std::size_t port, float gap, float wallVelocity, float restingVelocity, Values &v,
              const Values &q) noexcept {
    if (inverseMass_[port] < 1.e-20f) return;
    const auto &w = parameters_.motion > 0 ? movingWeights_[port] : weights_[port];
    double position = 0, speed = 0;
    double mass = 0;
    for (std::size_t i = 0; i < active_; ++i) {
      position += double(w[i]) * q[i] * inverseOmega_[i];
      speed += double(w[i]) * v[i];
      mass += double(w[i]) * w[i];
    }
    if (mass < 1.e-20) return;
    RattleImpact(port, gap, wallVelocity, position, speed, mass, w, v);
    if (!rattle_[port].Moving() && position >= gap)
      RestingContact(restingVelocity, mass, w, v);
  }

  void RestingContact(float drive, double mass, const Values &w, Values &v) noexcept {
    double speed = 0;
    for (std::size_t i = 0; i < active_; ++i) speed += double(w[i]) * v[i];
    const double relative = speed - drive;
    // Coulomb-like contact drag, only while a rim point actually touches.
    // Saturating the impulse at zero relative velocity prevents sign reversal
    // or energy creation. This is state damping, never an output audio gate.
    const double limit = ReturnAcceleration() * parameters_.loss / rate_;
    const double impulse = -std::clamp(relative / mass, -limit, limit);
    for (std::size_t i = 0; i < active_; ++i) v[i] += float(impulse * w[i]);
    work_ += impulse * drive;
    dissipated_ -= impulse * relative + .5 * impulse * impulse * mass;
  }

  void RattleImpact(std::size_t port, float gap, float drive, double position,
                    double speed, double mass, const Values &w, Values &v) noexcept {
    auto &state = rattle_[port];
    const double acceleration = ReturnAcceleration();
    const bool returning = state.Advance(acceleration, 1.0 / rate_);
    // Existing rim overlap launches contact motion. Once launched, returning
    // motion belongs to the contacted patch, rather than retesting a fixed gap.
    // This retained contact is a constructive approximation, not shell geometry.
    // Both impulse directions use the same coupling, preserving reciprocity
    // and energy. Settling changes return force, NOT the freedom to bounce.
    // Closing removes the freedom to bounce; the closed endpoint becomes a
    // dissipative fixed boundary, not an open rattle with a smaller launch gap.
    const double coupling = pedal_.Current();
    const double relative = speed - drive - coupling * state.velocity;
    const double height = coupling * state.potential / acceleration;
    const bool touching = position >= gap + height;
    const bool closing = drive < 0 && position >= gap;
    if (!returning && ((!touching && !closing) || relative <= 0)) return;
    if (relative > 0) {
      const double inverseMass = mass + coupling * coupling;
      const double impulse = -(1.0 + restitution_) * relative / inverseMass;
      for (std::size_t i = 0; i < active_; ++i) v[i] += float(impulse * w[i]);
      state.velocity -= coupling * impulse;
      work_ += impulse * drive;
      dissipated_ += .5 * (1 - double(restitution_) * restitution_) *
                    relative * relative / inverseMass;
      ++collisions_;
    }
    dissipated_ += state.Capture(acceleration, 1.0 / rate_);
  }

  void UpdateMotion() noexcept {
    // Control-rate spatial motion. Each collision is passive for ANY vector,
    // including time-varying vectors; this never rotates stored modal states.
    // Smooth random spatial motion defines texture, not a hit timer or LFO.
    if (parameters_.motion <= 0 || motionSample_++ % 32 != 0) return;
    for (std::size_t p = 0; p < PortCount; ++p) {
      const float mix = .5f * parameters_.motion * (1 + motion_[p].Next());
      // The same bounded rocking coordinate sweeps the local rim clearance
      // from +gap toward -gap (touching/preloaded), not a separate LFO knob.
      // Geometry controls collision timing; all impulses remain passive.
      gapScale_[p] = 1 - 2 * mix;
      double norm = 0;
      for (std::size_t i = 0; i < active_; ++i) {
        const float w = (1 - mix) * weights_[p][i] + mix * weights_[(p + 1) % PortCount][i];
        movingWeights_[p][i] = w;
        norm += double(w) * w;
      }
      const float scale = norm > 1.e-20 ? float(1 / std::sqrt(norm)) : 0;
      for (std::size_t i = 0; i < active_; ++i) movingWeights_[p][i] *= scale;
    }
  }

  std::array<Values, PortCount> weights_{};
  std::array<Values, PortCount> movingWeights_{};
  std::array<RimRattleState, PortCount> rattle_{};
  std::array<tfdsp::SmoothRandomModulator, PortCount> motion_{};
  std::array<float, PortCount> gapScale_{};
  unsigned motionSample_{};
  std::array<float, PortCount> inverseMass_{};
  Values inverseOmega_{};
  ModalRimContactParameters parameters_{};
  LiveGain pedal_{};
  LiveGain settling_{};
  float rate_{48000.f}, restitution_{std::sqrt(.75f)};
  std::size_t active_{};
  double work_{}, dissipated_{};
  unsigned collisions_{};
};
} // namespace tfdsp::percussion
