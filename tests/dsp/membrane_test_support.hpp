#pragma once
#include "percussion_test_support.hpp"

#include "tfdsp/percussion/fixed_mixer.hpp"
#include "tfdsp/percussion/kick_voice_parameters.hpp"
#include "tfdsp/percussion/membrane_drum.hpp"
#include "tfdsp/percussion/membrane_resonator.hpp"
#include "tfdsp/percussion/observation_equalizer.hpp"
#include "tfdsp/percussion/strike_energy_envelope.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace membrane_test {
using percussion_test::Check;
using percussion_test::CheckNear;

inline double Energy(const std::vector<float> &audio) {
  double result = 0.;
  for (const float sample : audio)
    result += sample * sample;
  return result;
}

inline std::vector<float>
Render(const float sampleRate, const tfdsp::percussion::MembraneDrumHit &hit,
       const tfdsp::percussion::MembraneDrumParameters &p) {
  tfdsp::percussion::MembraneDrum drum;
  drum.Prepare(sampleRate, p);
  drum.Trigger(hit);
  std::vector<float> output(static_cast<std::size_t>(sampleRate));
  for (float &sample : output)
    sample = drum.Process();
  return output;
}

inline std::size_t ZeroCrossings(const float tensionScale) {
  using Resonator = tfdsp::percussion::MembraneResonator<1>;
  Resonator::Parameters parameters{{{200.f, 2.f, 1.f, 1.f, 1.f, 1.f}}};
  Resonator resonator;
  resonator.Prepare(48000.f, parameters);
  Resonator::Drive drive{1.f};
  float previous = resonator.Process(drive, tensionScale);
  std::size_t crossings = 0;
  drive[0] = 0.f;
  for (std::size_t sample = 1; sample < 4800; ++sample) {
    const float current = resonator.Process(drive, tensionScale);
    if ((current < 0.f) != (previous < 0.f))
      ++crossings;
    previous = current;
  }
  return crossings;
}

void TestMixerAndStrikeEnergy();
void TestDynamicTension();
void TestEqualizerModes();
void TestMembraneRecipe();
void TestVelocityIsLinear();
void TestSingleHitEnergyBudget();
void TestAcousticKickEnergyBudget();
void TestDefaultHeadroom();
void TestRatesAndRoutes();
void TestIndependentFmPitchTime();
void TestFmDepthActuallyDecaysGeometrically();
void TestIndependentContactNoise();
void TestKickObservationPaths();
void TestKickDecayShapes();
void TestUnifiedKickSurface();
void TestKickQuietModeRemovalIsContinuous();
} // namespace membrane_test
