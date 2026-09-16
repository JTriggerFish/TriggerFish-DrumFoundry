#pragma once
#include "percussion_test_support.hpp"

#include "tfdsp/percussion/crash_cymbal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

using percussion_test::Check;
using percussion_test::CheckNear;

namespace crash_test {

inline std::vector<float> Render(const float strength, const float location,
                                 const float hardness, const std::uint32_t seed,
                                 const float seconds = 2.f,
                                 const float sampleRate = 48000.f) {
  using namespace tfdsp::percussion;
  CrashCymbal cymbal;
  cymbal.Prepare(sampleRate, DefaultCrashCymbalParameters(sampleRate));
  cymbal.Trigger({strength, location, hardness, seed});
  std::vector<float> output(static_cast<std::size_t>(seconds * sampleRate));
  for (float &sample : output)
    sample = cymbal.Process();
  return output;
}

inline double Energy(const std::vector<float> &audio) {
  double energy = 0.0;
  for (const float sample : audio)
    energy += static_cast<double>(sample) * sample;
  return energy;
}

inline double Difference(const std::vector<float> &first,
                         const std::vector<float> &second) {
  double energy = 0.0;
  for (std::size_t sample = 0; sample < first.size(); ++sample) {
    const double delta = first[sample] - second[sample];
    energy += delta * delta;
  }
  return energy;
}

inline double NormalizedDifferenceEnergy(const std::vector<float> &audio) {
  double signal = 0.0;
  double difference = 0.0;
  float previous = 0.f;
  for (const float sample : audio) {
    signal += static_cast<double>(sample) * sample;
    const double delta = sample - previous;
    difference += delta * delta;
    previous = sample;
  }
  return difference / std::max(signal, 1.e-30);
}

void TestDeterministicAndResponsive();
void TestVelocityEnergy();
void TestVelocityChangesCymbalRegime();
void TestSparseModesArePlacedDirectly();
void TestBodyDecayCurveUsesActiveErbKnots();
void TestImplementFamiliesAreDistinct();
void TestDefaultBodyCoversTheMeasuredLowRegion();
void TestUnifiedFieldExpandsAnchorsWithoutChangingDriveEnergy();
void TestPairedRingIsNormalizedAndKeepsItsPitch();
void TestPairedRingMatchesAnalyticAudio();
void TestPairedRingDepthAndRateTilt();
void TestPaintedLevelsOnlyShapeNormalizedObservation();
void TestExcitationShelfCentreChangesNormalizedColour();
void TestUnifiedFieldAcceptsConstructiveAnchorEditing();
void TestContactCalibrationMacrosAreAudible();
void TestMuteIsPassive();
void TestFiniteAtSupportedRates();
void TestAnalysisFrameMatchesOutput();
void TestBloomIsIntrinsicUpwardEnergyTransport();
void TestStrongerStrikeAcceleratesUpwardEnergyTransport();
void TestZeroStrengthTriggerIsANoOp();
void TestRestrikeAddsWithoutRecolouringStoredEnergy();
void TestRepeatedHitsAccumulateBodyEnergy();
void TestSharedOutputEq();
void TestUpperModalRangeAndDecay();
void TestMaximumBloomRemainsBounded();
} // namespace crash_test
