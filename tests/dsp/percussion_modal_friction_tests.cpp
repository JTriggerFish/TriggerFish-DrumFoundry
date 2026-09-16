#include "percussion_test_support.hpp"
#include "tfdsp/percussion/stochastic_modal_field.hpp"

using namespace tfdsp::percussion;
using percussion_test::Check;
using percussion_test::CheckNear;

namespace {
void TestShrink() {
  std::array<float, 3> re{3, .01f, 0}, im{4, -.01f, 0}, weights{1, 1, 1};
  ApplyModalFrictionLoss(re, im, weights, 3, 0.f);
  Check(re[0] == 3 && im[0] == 4, "zero loss is bit-exact bypass");
  ApplyModalFrictionLoss(re, im, weights, 3, .1f);
  CheckNear(std::hypot(re[0], im[0]), 4.9, 1.e-6, "radial amplitude drain");
  CheckNear(re[0] / im[0], .75, 1.e-6, "phase is unchanged");
  Check(re[1] == 0 && im[1] == 0, "weak mode stops without negative energy");
  Check(re[2] == 0 && im[2] == 0, "silence is never excited");
  re[1] = 1;
  ApplyModalFrictionLoss(re, im, weights, 3, .1f);
  CheckNear(re[1], .9, 1.e-6, "a stopped mode revives without a latch");
  std::array<float, 2> amplitudes{1, .25f}, phases{}, density{1, 1};
  for (int i = 0; i < 30; ++i)
    ApplyModalFrictionLoss(amplitudes, phases, density, 2, .01f);
  Check(amplitudes[1] == 0 && amplitudes[0] > .69f,
        "softer strikes end sooner without compressing excitation");
}

void TestSplit(float rate, int count) {
  std::array<float, 512> re{}, im{}, weights{};
  const float weight = 1.f / std::sqrt(float(count));
  for (int i = 0; i < count; ++i) re[i] = weights[i] = weight;
  const float lambda = std::log(1000.f) / 2.f;
  const float pole = std::exp(-lambda / rate);
  for (int n = 0; n < int(.4f * rate); ++n) {
    for (int i = 0; i < count; ++i) re[i] *= pole;
    ApplyModalFrictionLoss(re, im, weights, count, .2f / rate);
  }
  double energy = 0;
  for (int i = 0; i < count; ++i) energy += double(re[i])*re[i];
  const double expected = (1 + .2 / lambda)*std::exp(-lambda * .4) - .2/lambda;
  CheckNear(std::sqrt(energy), expected, .0006,
            "mixed viscous/friction law is rate and subdivision invariant");
}

void TestField(float rate) {
  StochasticModalField<1> field;
  StochasticModalField<1>::Parameters p{};
  p[0].frequencyHz = 1000;
  p[0].decaySeconds = 2;
  p[0].inputGain = p[0].outputGain = 1;
  field.Prepare(rate, p, {}, 700, 6500);
  field.SetTailDamping(.2f, true);
  field.ProcessExcitedPair(1, 0);
  double prior = field.StoredEnergy();
  for (int i = 0; i < int(2*rate); ++i) {
    field.ProcessExcitedPair(0, 0);
    const auto e = field.StoredEnergy();
    Check(e <= prior + 1.e-7, "loss cannot inject energy");
    prior = e;
  }
  Check(prior == 0, "field reaches exact silence in finite time");
  field.ProcessExcitedPair(.5f, 0);
  Check(field.StoredEnergy() > .24, "restrike restores energy normally");
  StochasticModalField<1> edited;
  edited.Prepare(rate, p, {}, 700, 6500);
  Check(edited.RetainState(field), "live edit accepted");
  for (int i = 0; i < int(2*rate); ++i) edited.ProcessExcitedPair(0, 0);
  Check(edited.StoredEnergy() == 0, "prepared edit retains friction setting");
}
} // namespace

int main() {
  TestShrink();
  for (float rate : {44100.f, 48000.f, 96000.f}) {
    for (int count : {1, 8, 64, 512}) TestSplit(rate, count);
    TestField(rate);
  }
  return percussion_test::failures == 0 ? 0 : 1;
}
