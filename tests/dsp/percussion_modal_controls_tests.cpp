#include "modal_test_support.hpp"

namespace modal_test {

void TestStochasticPhaseBroadeningPreservesEnergy() {
  using Field = tfdsp::percussion::StochasticModalField<1>;
  Field::Parameters coherent{{
      {1200.f, 2.f, 1.f, 1.f, 0.f, 0.f, 0},
  }};
  auto diffused = coherent;
  diffused[0].phaseBandwidthHz = 1800.f;
  Field first;
  Field second;
  first.Prepare(48000.f, coherent, {}, 700.f, 6500.f);
  second.Prepare(48000.f, diffused, {}, 700.f, 6500.f);
  double outputDifference = 0.0;
  constexpr int frames = 4096;
  constexpr double floatEpsilon = 1.1920928955078125e-7;
  const double expectedEnergyRatio = std::exp(2 * std::log(.001) / (2 * 48000));
  for (int sample = 0; sample < frames; ++sample) {
    const double before = second.StoredEnergy();
    const float input = sample == 0 ? 1.f : 0.f;
    const double coherentOutput = first.ProcessExcitedPair(input, 0.f);
    const double diffusedOutput = second.ProcessExcitedPair(input, 0.f);
    const double difference = coherentOutput - diffusedOutput;
    outputDifference += difference * difference;
    if (sample > 0)
      CheckNear(second.StoredEnergy() / before, expectedEnergyRatio,
                8 * floatEpsilon,
                "phase rotation preserves each step's damping");
  }
  Check(outputDifference > 1.0,
        "phase bandwidth audibly decorrelates a modal ridge");
  // Float rotations accumulate rounding differently with ARM fused arithmetic.
  // Bound the long-run difference by one float epsilon per step, alongside the
  // tighter local damping check above; do not change the synthesis to fit a
  // CPU.
  CheckNear(second.StoredEnergy() / first.StoredEnergy(), 1.0,
            frames * floatEpsilon,
            "phase broadening changes coherence without changing energy");
}

void TestLocalModalExchangeIsPassive() {
  using Field = tfdsp::percussion::StochasticModalField<4>;
  Field::Parameters parameters{{
      {700.f, 3.f, 1.f, 1.f, 0.f, 0.f, 0},
      {900.f, 3.f, .7f, 1.f, .4f, 0.f, 0},
      {1300.f, 3.f, .5f, 1.f, -.3f, 0.f, 1},
      {1700.f, 3.f, .3f, 1.f, .8f, 0.f, 1},
  }};
  Field independent;
  Field coupled;
  independent.Prepare(48000.f, parameters, {}, 600.f, 1500.f);
  coupled.Prepare(48000.f, parameters, {.02f, 17}, 600.f, 1500.f);
  double outputDifference = 0.0;
  double maximumSpontaneousGrowth = 0.0;
  double priorCoupledEnergy = 0.0;
  for (int sample = 0; sample < 4096; ++sample) {
    const float input = sample == 0 ? 1.f : 0.f;
    const double independentOutput = independent.ProcessExcitedPair(input, 0.f);
    const double coupledOutput = coupled.ProcessExcitedPair(input, 0.f);
    const double difference = independentOutput - coupledOutput;
    outputDifference += difference * difference;
    const double energy = coupled.StoredEnergy();
    if (sample > 0)
      maximumSpontaneousGrowth =
          std::max(maximumSpontaneousGrowth, energy - priorCoupledEnergy);
    priorCoupledEnergy = energy;
  }
  Check(outputDifference > 1.0,
        "local exchange moves energy within and between adjacent packets");
  CheckNear(coupled.StoredEnergy() / independent.StoredEnergy(), 1.0, 2.e-4,
            "local Givens exchange preserves stored modal energy");
  Check(maximumSpontaneousGrowth < 2.e-6,
        "local exchange never creates unforced modal energy");
}

void TestModalFieldRejectsSplitPacketRuns() {
  using namespace tfdsp::percussion;
  using Field = StochasticModalField<3>;
  Field::Parameters parameters{{
      {500.f, 1.f, 1.f, 1.f, 0.f, 0.f, 7, 1.f},
      {1000.f, 1.f, 1.f, 1.f, 0.f, 0.f, 8, 1.f},
      {2000.f, 1.f, 1.f, 1.f, 0.f, 0.f, 7, 1.f},
  }};
  bool rejected = false;
  try {
    (void)PrepareStochasticModalField(48000.f, parameters, {}, 700.f, 6500.f);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  Check(rejected, "modal field rejects non-contiguous packet membership");
}

void TestModalPacketAllocationUsesOneSharedBoundedPool() {
  using namespace tfdsp::percussion;
  std::array<ModalPacketRequest, 32> requests{};
  for (std::size_t index = 0; index < 12; ++index)
    requests[index] = {5.f + static_cast<float>(index), 6.f, true};
  const auto centresOnly = AllocateModalPackets(requests, 512, 0.f);
  const auto dense = AllocateModalPackets(requests, 512, 1.f);
  const auto paired = AllocateModalPackets(requests, 512, 0.f, true);
  const auto pairedDense = AllocateModalPackets(requests, 512, 1.f, true);
  Check(paired.stateCount == 24 && pairedDense.stateCount == 512,
        "paired centres reserve two states without exceeding the shared pool");
  Check(centresOnly.activeHandleCount == 12 && centresOnly.stateCount == 12,
        "zero satellite density retains only painted centre handles");
  Check(dense.activeHandleCount == 12 && dense.stateCount <= 512 &&
            dense.stateCount > centresOnly.stateCount,
        "satellites share one bounded modal-state pool");
  Check(dense.stateCount == 512, "full density uses the shared pool");
  for (auto &request : requests)
    request.spreadErb = .02f;
  const auto narrow = AllocateModalPackets(requests, 512, 1.f);
  Check(narrow.stateCount == 512,
        "narrow stable packets are not forced sparse");
  requests[0].allocationWeight = 0;
  requests[1].allocationWeight = 4;
  const auto weighted = AllocateModalPackets(requests, 512, 1.f);
  Check(weighted.sidebandPairs[0] == 0 && weighted.stateCount == 512,
        "zero local weight reallocates without wasting the pool");
  Check(weighted.sidebandPairs[1] >= 3 * weighted.sidebandPairs[2],
        "local weights give controllable relative allocation");
  for (std::size_t index = 12; index < requests.size(); ++index)
    Check(dense.sidebandPairs[index] == 0,
          "inactive handles never consume sideband states");
}

} // namespace modal_test
