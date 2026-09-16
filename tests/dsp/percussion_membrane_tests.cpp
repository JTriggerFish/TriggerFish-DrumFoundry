#include "membrane_test_support.hpp"

using namespace membrane_test;

int main() {
  TestMixerAndStrikeEnergy();
  TestEqualizerModes();
  TestDynamicTension();
  TestMembraneRecipe();
  TestVelocityIsLinear();
  TestSingleHitEnergyBudget();
  TestAcousticKickEnergyBudget();
  TestDefaultHeadroom();
  TestRatesAndRoutes();
  TestIndependentFmPitchTime();
  TestFmDepthActuallyDecaysGeometrically();
  TestIndependentContactNoise();
  TestUnifiedKickSurface();
  TestKickQuietModeRemovalIsContinuous();
  if (percussion_test::failures == 0)
    std::cout << "All membrane percussion tests passed\n";
  return percussion_test::failures == 0 ? 0 : 1;
}
