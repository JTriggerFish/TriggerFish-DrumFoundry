#include "modal_test_support.hpp"

using namespace modal_test;

int main() {
  TestOneModeMatchesAnalyticRecurrence();
  TestIndependentProjections();
  TestPreparedProjectionIsSanitizedOnce();
  TestTwoExcitationsShareOneStoredBody();
  TestEnergyNormalizedProjectionOnlyRedistributesDrive();
  TestEnergyNormalizedProjectionSurvivesStaticRebuild();
  TestModalConstraintReferenceGain();
  TestStatisticalCloudIsDeterministicAndNormalized();
  TestDenseCloudRemainsFinite();
  TestDenseCloudGainEnvelopeShapesBroadBands();
  TestDenseCloudDecayEnvelopeShapesMiddleBand();
  TestDenseCloudDensityPreservesPlacementAndLevel();
  TestTurbulentResidualStoresAndPassivelyLosesEnergy();
  TestTurbulentResidualIsRepeatable();
  TestStochasticPhaseBroadeningPreservesEnergy();
  TestLocalModalExchangeIsPassive();
  TestModalCascadeIsPassiveAndDoesNotRetransferArrivals();
  TestModalCascadeIsIndependentOfPaintedAnchorDensity();
  TestModalCascadePreservesPacketTurbulenceWeights();
  TestModalFieldRejectsSplitPacketRuns();
  TestModalCascadeRateIncludesVerySlowTravel();
  TestModalCascadeEnergyAccelerationOnlyAccelerates();
  TestModalPacketAllocationUsesOneSharedBoundedPool();
  if (percussion_test::failures == 0)
    std::cout << "All percussion modal tests passed\n";
  return percussion_test::failures == 0 ? 0 : 1;
}
