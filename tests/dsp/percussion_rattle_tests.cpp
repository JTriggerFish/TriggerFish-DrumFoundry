#include "percussion_test_support.hpp"
#include "tfdsp/percussion/modal_rim_contact.hpp"
#include "tfdsp/percussion/stochastic_modal_field.hpp"
using namespace tfdsp::percussion;
using percussion_test::Check;
using percussion_test::CheckNear;

namespace {
template <std::size_t N> double Energy(const std::array<float, N> &v,
                                      const std::array<float, N> &q) {
  double e = 0;
  for (unsigned i = 0; i < N; ++i) e += .5 * (double(v[i])*v[i] + double(q[i])*q[i]);
  return e;
}

void Flight(float rate) {
  RimRattleState state{.1, 0};
  const double dt = 1.0 / rate, acceleration = .5;
  double interval = 0, previous = 1e9;
  unsigned impacts = 0;
  for (unsigned i = 0; i < unsigned(2 * rate) && state.Moving(); ++i) {
    const double before = state.Energy();
    interval += dt;
    if (state.Advance(acceleration, dt)) {
      Check(interval < previous, "diminishing bounces arrive progressively faster");
      previous = interval;
      interval = 0;
      ++impacts;
      state.velocity *= -.6;
      state.Capture(acceleration, dt);
    } else {
      CheckNear(state.Energy(), before, 1e-12, "ballistic motion preserves energy");
    }
  }
  Check(impacts > 5 && !state.Moving(), "inelastic rattle reaches rest");
}

void Budget(float rate, bool pedal, float movement, float settling) {
  constexpr unsigned n = 16;
  ModalRimContact<n> contact;
  std::array<float, n> f{}, w{}, v{}, q{}, c{}, s{};
  std::array<std::uint32_t, n> ids{};
  for (unsigned i = 0; i < n; ++i) {
    f[i] = 100.f + 700.f * i;
    w[i] = .25f;
    ids[i] = i + 1;
    v[i] = .1f * std::sin(float(i));
    c[i] = std::cos(6.28318530718f * f[i] / rate);
    s[i] = std::sin(6.28318530718f * f[i] / rate);
  }
  contact.Prepare(rate, f, w, ids, n);
  ModalRimContactParameters p{true, pedal ? 1.f : .5f, .001f, .5f, .1f, movement, settling};
  contact.SetParameters(p, true);
  unsigned impacts = 0;
  for (unsigned sample = 0; sample < unsigned(rate); ++sample) {
    for (unsigned i = 0; i < n; ++i) {
      const float old = v[i];
      v[i] = c[i]*v[i] - s[i]*q[i];
      q[i] = s[i]*old + c[i]*q[i];
    }
    if (pedal && sample == 100) { p.openness = 0; contact.SetParameters(p); }
    if (sample == 200) {
      const double stored = contact.StoredEnergy();
      p.settling = 0;
      contact.SetParameters(p);
      CheckNear(contact.StoredEnergy(), stored, 0, "zero settling preserves rattle state");
    }
    if (sample == 400) { p.settling = 1; contact.SetParameters(p); }
    const double before = Energy(v, q) + contact.StoredEnergy();
    contact.Process(v, q);
    const double after = Energy(v, q) + contact.StoredEnergy();
    CheckNear(after-before, contact.LastWork()-contact.LastDissipation(),
              2.e-7, "modal plus rattle energy equals external work minus loss");
    if (!pedal) Check(after <= before + 2e-7, "moving contact remains passive");
    impacts += contact.LastCollisions();
  }
  Check(impacts > 10, "rattle really exchanges energy");
  contact.Reset();
  v = {}; q = {};
  for (unsigned i = 0; i < 1000; ++i) contact.Process(v, q);
  Check(Energy(v, q) + contact.StoredEnergy() == 0, "settling never excites silence");
}

double PedalEnergy(float duration) {
  StochasticModalField<16> field;
  StochasticModalField<16>::Parameters modes{};
  for (unsigned i = 0; i < modes.size(); ++i) {
    modes[i].frequencyHz = 400 + 700.f*i;
    modes[i].decaySeconds = 3;
    modes[i].inputGain = modes[i].outputGain = .25f;
    modes[i].packetIdentity = i+1;
  }
  field.Prepare(48000, modes, {}, 700, 6500);
  ModalRimContactParameters p{true, 1, .0004f, .62f, .1f, .7f, .99f};
  field.SetRimContact(p, true);
  const int steps = std::max(1, int(duration*1000));
  double peak = 0;
  for (int step = 1; step <= steps; ++step) {
    p.openness = 1.f - float(step)/steps;
    field.SetRimContact(p);
    for (int i = 0; i < 48; ++i) {
      field.ProcessExcitedPair(0,0);
      peak = std::max(peak, field.StoredEnergy());
    }
  }
  for (int i = 0; i < 48000; ++i) {
    field.ProcessExcitedPair(0,0);
    peak = std::max(peak, field.StoredEnergy());
  }
  return peak;
}
} // namespace

int main() {
  for (float rate : {44100.f, 48000.f, 96000.f}) {
    Flight(rate);
    for (float motion : {0.f, .7f, 1.f}) {
      for (float settling : {0.f, .001f, .5f, 1.f}) {
        Budget(rate, false, motion, settling);
        Budget(rate, true, motion, settling);
      }
    }
  }
  const double fast = PedalEnergy(.005f), slow = PedalEnergy(1.f);
  Check(fast > 0 && slow < .01*fast, "settling preserves quiet slow pedal closure");
  return percussion_test::failures ? 1 : 0;
}
