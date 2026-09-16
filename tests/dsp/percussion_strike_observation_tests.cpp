#include "percussion_test_support.hpp"
#include "tfdsp/percussion/crash_cymbal.hpp"
#include <memory>

using namespace tfdsp::percussion;
using percussion_test::Check;
using percussion_test::CheckNear;

namespace {
void TestOverflow() {
  ObservedNoiseStrike full, extra;
  full.Prepare(48000);
  extra.Prepare(48000);
  for (unsigned seed = 1; seed <= 8; ++seed) {
    full.Trigger(1, .12f, 0, seed);
    extra.Trigger(1, .12f, 0, seed);
  }
  for (int i = 0; i < 100; ++i) {
    if (i == 20)
      extra.Trigger(1, .12f, 24, 999);
    Check(full.Process() == extra.Process(),
          "overflow does not cut or restart a sounding contact");
  }
}

void TestDecayAndVelocity(float rate) {
  double early = 0, late = 0;
  for (unsigned seed = 1; seed <= 32; ++seed) {
    ObservedNoiseStrike loud, soft;
    loud.Prepare(rate);
    soft.Prepare(rate);
    loud.Trigger(1, .03f, 0, seed);
    soft.Trigger(.25f, .03f, 0, seed);
    for (int i = 0; i < int(.035f * rate); ++i) {
      const double x = loud.Process();
      CheckNear(soft.Process(), .25 * x, 1.e-7,
                "velocity is linear, without compression or normalization");
      if (i >= int(.001f * rate) && i < int(.003f * rate))
        early += x * x;
      if (i >= int(.031f * rate) && i < int(.033f * rate))
        late += x * x;
    }
  }
  CheckNear(10 * std::log10(late / early), -60, 1,
            "displayed T60 is the measured power decay");
}

void TestEnvelope(float rate) {
  ObservedNoiseStrike a, b, sum;
  a.Prepare(rate);
  b.Prepare(rate);
  sum.Prepare(rate);
  a.Trigger(.4f, .03f, 12.f, 51);
  sum.Trigger(.4f, .03f, 12.f, 51);
  bool audible = false;
  for (int i = 0; i < int(.2f * rate); ++i) {
    if (i == int(.01f * rate)) {
      b.Trigger(.7f, .04f, -8.f, 98);
      sum.Trigger(.7f, .04f, -8.f, 98);
    }
    const float x = sum.Process();
    CheckNear(x, a.Process() + b.Process(), 1.e-7,
              "overlapping contacts retain their own colour and envelope");
    Check(std::isfinite(x), "strike stays finite");
    audible |= std::abs(x) > .01f;
    if (i > int(.1f * rate))
      Check(x == 0.f, "strike ends; no extra noise tail");
  }
  Check(audible, "strike is audible");
  sum.Trigger(1, .12f, 24, 2);
  sum.Reset();
  Check(sum.Process() == 0, "reset clears every overlapping accent");
}

void TestBodyIsolation(float rate, bool route) {
  CrashCymbalFitParameters original;
  original.outputEqEnabled = false;
  auto changed = original;
  changed.observedNoiseLevel = 1.2f;
  changed.observedNoiseDecaySeconds = .03f;
  changed.observedNoiseColourDb = 18.f;
  auto pa = DefaultCrashCymbalParameters(rate, original);
  auto pb = DefaultCrashCymbalParameters(rate, changed);
  const auto observation = std::size_t(MetallicPlateRoute::ContactToObservation);
  pa.routing.SetEnabled(observation, route);
  pb.routing.SetEnabled(observation, route);
  auto a = std::make_unique<CrashCymbal>();
  auto b = std::make_unique<CrashCymbal>();
  a->Prepare(rate, pa);
  b->Prepare(rate, pb);
  double difference = 0;
  for (int i = 0; i < int(.5f * rate); ++i) {
    if (i == 0 || i == int(.015f * rate) || i == int(.12f * rate)) {
      const CrashCymbalHit hit{.75f, .5f, .65f, unsigned(i + 1), 1.f, .2f};
      a->Trigger(hit);
      b->Trigger(hit);
    }
    const auto x = a->ProcessFrame(), y = b->ProcessFrame();
    Check(x.modalBody == y.modalBody, "accent cannot change any body sample");
    Check(a->StoredBodyEnergy() == b->StoredBodyEnergy(),
          "accent cannot change stored body energy");
    difference += std::abs(x.output - y.output);
    if (i > int(.2f * rate) || !route)
      Check(x.output == y.output, "tail and disconnected observation are exact");
  }
  Check(route ? difference > 1 : difference == 0,
        "accent follows only the contact observation route");
}
} // namespace

int main() {
  TestOverflow();
  for (float rate : {44100.f, 48000.f, 96000.f}) {
    TestDecayAndVelocity(rate);
    TestEnvelope(rate);
    TestBodyIsolation(rate, true);
    TestBodyIsolation(rate, false);
  }
  return percussion_test::failures ? 1 : 0;
}
