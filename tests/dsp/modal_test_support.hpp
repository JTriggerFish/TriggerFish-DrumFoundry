#pragma once
#include "percussion_test_support.hpp"

#include "tfdsp/percussion/modal_bank.hpp"
#include "tfdsp/percussion/modal_constraint.hpp"
#include "tfdsp/percussion/modal_energy_cascade.hpp"
#include "tfdsp/percussion/modal_packet_allocator.hpp"
#include "tfdsp/percussion/statistical_modal_cloud.hpp"
#include "tfdsp/percussion/stochastic_modal_field.hpp"
#include "tfdsp/percussion/turbulent_residual.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>

namespace modal_test {
using percussion_test::Check;
using percussion_test::CheckNear;

void TestOneModeMatchesAnalyticRecurrence();
void TestIndependentProjections();
void TestPreparedProjectionIsSanitizedOnce();
void TestTwoExcitationsShareOneStoredBody();
void TestEnergyNormalizedProjectionOnlyRedistributesDrive();
void TestEnergyNormalizedProjectionSurvivesStaticRebuild();
void TestModalConstraintReferenceGain();
void TestStatisticalCloudIsDeterministicAndNormalized();
void TestDenseCloudRemainsFinite();
void TestDenseCloudGainEnvelopeShapesBroadBands();
void TestDenseCloudDecayEnvelopeShapesMiddleBand();
void TestDenseCloudDensityPreservesPlacementAndLevel();
void TestTurbulentResidualStoresAndPassivelyLosesEnergy();
void TestTurbulentResidualIsRepeatable();
void TestStochasticPhaseBroadeningPreservesEnergy();
void TestLocalModalExchangeIsPassive();
void TestModalCascadeIsPassiveAndDoesNotRetransferArrivals();
void TestModalCascadeIsIndependentOfPaintedAnchorDensity();
void TestModalCascadePreservesPacketTurbulenceWeights();
void TestModalFieldRejectsSplitPacketRuns();
void TestModalCascadeRateIncludesVerySlowTravel();
void TestModalCascadeEnergyAccelerationOnlyAccelerates();
void TestModalPacketAllocationUsesOneSharedBoundedPool();
} // namespace modal_test
