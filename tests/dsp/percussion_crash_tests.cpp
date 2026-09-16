#include "crash_test_support.hpp"

using namespace crash_test;

int main() {
  TestDeterministicAndResponsive();
  TestVelocityEnergy();
  TestVelocityChangesCymbalRegime();
  TestSparseModesArePlacedDirectly();
  TestBodyDecayCurveUsesActiveErbKnots();
  TestImplementFamiliesAreDistinct();
  TestDefaultBodyCoversTheMeasuredLowRegion();
  TestUnifiedFieldExpandsAnchorsWithoutChangingDriveEnergy();
  TestPairedRingIsNormalizedAndKeepsItsPitch();
  TestPairedRingMatchesAnalyticAudio();
  TestPairedRingDepthAndRateTilt();
  TestPaintedLevelsOnlyShapeNormalizedObservation();
  TestExcitationShelfCentreChangesNormalizedColour();
  TestUnifiedFieldAcceptsConstructiveAnchorEditing();
  TestContactCalibrationMacrosAreAudible();
  TestMuteIsPassive();
  TestFiniteAtSupportedRates();
  TestAnalysisFrameMatchesOutput();
  TestBloomIsIntrinsicUpwardEnergyTransport();
  TestStrongerStrikeAcceleratesUpwardEnergyTransport();
  TestZeroStrengthTriggerIsANoOp();
  TestRestrikeAddsWithoutRecolouringStoredEnergy();
  TestRepeatedHitsAccumulateBodyEnergy();
  TestMaximumBloomRemainsBounded();
  TestUpperModalRangeAndDecay();
  TestSharedOutputEq();
  if (percussion_test::failures == 0)
    std::cout << "All percussion crash tests passed\n";
  return percussion_test::failures == 0 ? 0 : 1;
}
