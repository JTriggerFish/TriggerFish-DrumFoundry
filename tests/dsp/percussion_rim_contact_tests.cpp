#include "percussion_test_support.hpp"
#include "tfdsp/percussion/stochastic_modal_field.hpp"
using namespace tfdsp::percussion;
using percussion_test::Check;
using percussion_test::CheckNear;

namespace {
template <std::size_t N> double Energy(const std::array<float, N> &v) {
  double e = 0;
  for (float x : v) e += .5 * double(x) * x;
  return e;
}

void Budget(float rate, bool moving) {
  constexpr unsigned n = 32;
  ModalRimContact<n> contact;
  std::array<float, n> f{}, w{}, v{}, q{};
  std::array<std::uint32_t, n> ids{};
  for (unsigned i = 0; i < n; ++i) {
    f[i] = 100.f + 400.f * i;
    w[i] = 1.f / std::sqrt(float(n));
    ids[i] = i + 1;
    v[i] = std::sin(float(i));
  }
  contact.Prepare(rate, f, w, ids, n);
  ModalRimContactParameters p{true, moving ? 1.f : 0.f, .1f, .6f};
  contact.SetParameters(p, true);
  unsigned collisions = 0;
  double dissipated = 0;
  for (int sample = 0; sample < int(.1f * rate); ++sample) {
    for (unsigned i = 0; i < n; ++i) {
      const float angle = 6.28318530718f * f[i] / rate;
      const float previous = v[i];
      v[i] = std::cos(angle) * v[i] - std::sin(angle) * q[i];
      q[i] = std::sin(angle) * previous + std::cos(angle) * q[i];
    }
    if (moving && sample == 10) {
      p.openness = 0;
      contact.SetParameters(p);
    }
    const double before = Energy(v) + Energy(q) + contact.StoredEnergy();
    contact.Process(v, q);
    const double after = Energy(v) + Energy(q) + contact.StoredEnergy();
    CheckNear(after - before, contact.LastWork() - contact.LastDissipation(),
              2.e-6 * std::max(1.0, before), "collision energy equals work minus loss");
    if (!moving) Check(after <= before + 1.e-6, "stationary contact is passive");
    dissipated += contact.LastDissipation();
    collisions += contact.LastCollisions();
  }
  Check(collisions > 10 && dissipated > 1, "contact actually processes ringing");
}

double Pedal(float rate, float duration) {
  StochasticModalField<16> field;
  StochasticModalField<16>::Parameters modes{};
  for (unsigned i = 0; i < modes.size(); ++i) {
    auto &m = modes[i];
    m.frequencyHz = 400 + 700.f * i;
    m.decaySeconds = 3;
    m.inputGain = m.outputGain = .25;
    m.packetIdentity = i + 1;
  }
  field.Prepare(rate, modes, {}, 700, 6500);
  ModalRimContactParameters p{true, 1, .1f, .7f};
  field.SetRimContact(p, true);
  for (int i = 0; i < 100; ++i) field.ProcessExcitedPair(0, 0);
  Check(field.StoredEnergy() == 0, "open silence stays silent");
  double peak = 0;
  const int steps = std::max(1, int(duration * rate / 48));
  for (int step = 1; step <= steps; ++step) {
    p.openness = 1.f - float(step) / steps;
    field.SetRimContact(p);
    for (int i = 0; i < 48; ++i) {
      field.ProcessExcitedPair(0, 0);
      peak = std::max(peak, field.StoredEnergy());
    }
  }
  for (int i = 0; i < int(.02f * rate); ++i) {
    field.ProcessExcitedPair(0, 0);
    peak = std::max(peak, field.StoredEnergy());
  }
  field.Reset();
  for (int i = 0; i < 500; ++i) field.ProcessExcitedPair(0, 0);
  Check(field.StoredEnergy() == 0, "reset with closed pedal supplies no energy");
  field.ProcessExcitedPair(1, 0);
  Check(field.StoredEnergy() > 0, "closed hat can be restruck");
  return peak;
}

void Disabled() {
  ModalRimContact<2> contact;
  std::array<float, 2> f{100, 1000}, w{1, 1}, v{.2f, -.1f}, q{1, 2};
  std::array<std::uint32_t, 2> ids{1, 2};
  contact.Prepare(48000, f, w, ids, 2);
  contact.SetParameters({false, 0, .1f, 1});
  const auto original = v;
  for (int i = 0; i < 300; ++i) contact.Process(v, q);
  Check(v == original, "disabled contact is exact bypass");
}

std::array<double, 2> Subdivide(unsigned members, float settling = 0, float motion = 0) {
  ModalRimContact<64> contact;
  std::array<float, 64> f{}, w{}, v{}, q{};
  std::array<std::uint32_t, 64> ids{};
  for (unsigned i = 0; i < 2 * members; ++i) {
    ids[i] = i / members + 1;
    f[i] = ids[i] == 1 ? 400.f : 3500.f;
    w[i] = 1.f / std::sqrt(float(2 * members));
    if (ids[i] == 1) v[i] = 1.f / std::sqrt(float(members));
  }
  contact.Prepare(48000, f, w, ids, 2 * members);
  contact.SetParameters({true, settling > 0 ? .5f : 0.f, 0.f, 0, 1, motion, settling}, true);
  const auto before = Energy(v);
  contact.Process(v, q);
  bool stored = contact.StoredEnergy() > 0;
  if (settling > 0) {
    // Motion can make every port initially receding. Exercise complete cycles,
    // not an assumption that this particular first sample must cause a hit.
    for (unsigned sample = 0; sample < 240; ++sample) {
      for (unsigned i = 0; i < 2*members; ++i) {
        const double a = 6.28318530718 * f[i] / 48000;
        const float old = v[i];
        v[i] = float(std::cos(a)*v[i] - std::sin(a)*q[i]);
        q[i] = float(std::sin(a)*old + std::cos(a)*q[i]);
      }
      contact.Process(v, q);
      stored |= contact.StoredEnergy() > 0;
    }
  }
  CheckNear(Energy(v) + Energy(q) + contact.StoredEnergy(), before, 1.e-5,
            "elastic contact redistributes without loss");
  std::array<double, 2> result{};
  for (unsigned i = 0; i < 2 * members; ++i)
    result[i / members] += double(v[i]) / std::sqrt(double(members));
  if (settling == 0)
    Check(std::abs(result[1]) > .01, "collision excites previously silent modes");
  else
    Check(stored, "rattle stores energy for returning impacts");
  return result;
}
} // namespace
int main() {
  Disabled();
  const auto single = Subdivide(1);
  for (unsigned count : {4u, 16u, 32u}) {
    const auto split = Subdivide(count);
    for (unsigned i = 0; i < 2; ++i)
      CheckNear(split[i], single[i], 1.e-5, "packet subdivision preserves contact mass");
  }
  const auto rattle = Subdivide(1, .8f, .7f);
  for (unsigned count : {4u, 16u, 32u}) {
    const auto split = Subdivide(count, .8f, .7f);
    for (unsigned i = 0; i < 2; ++i)
      CheckNear(split[i], rattle[i], 1.e-5, "rattle is invariant to equal subdivision");
  }
  for (float rate : {44100.f, 48000.f, 96000.f}) {
    Budget(rate, false);
    Budget(rate, true);
    const auto fast = Pedal(rate, 0);
    const auto slow = Pedal(rate, 1);
    Check(fast > 0 && slow < fast * .01, "slow closure supplies much less energy");
  }
  return percussion_test::failures ? 1 : 0;
}
