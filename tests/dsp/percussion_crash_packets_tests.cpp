#include "crash_test_support.hpp"

namespace crash_test {

void TestSparseModesArePlacedDirectly() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.sparseFrequencyHz[0] = 731.f;
  fit.sparseFrequencyHz[1] = 1193.f;
  fit.bodyDecaySeconds.fill(4.25f);
  const auto parameters = DefaultCrashCymbalParameters(48000.f, fit);
  const auto find = [&](const float frequency) {
    return std::min_element(parameters.modalField.begin(),
                            parameters.modalField.end(),
                            [frequency](const auto &left, const auto &right) {
                              return std::abs(left.frequencyHz - frequency) <
                                     std::abs(right.frequencyHz - frequency);
                            });
  };
  Check(std::abs(find(731.f)->frequencyHz - 731.f) < 1.e-5f &&
            std::abs(find(1193.f)->frequencyHz - 1193.f) < 1.e-5f,
        "crash anchor frequencies are independently and directly placed");
  Check(std::abs(find(731.f)->decaySeconds - 4.25f) < 1.e-5f,
        "crash anchor decay follows the shared body T60 curve");
}

void TestUnifiedFieldExpandsAnchorsWithoutChangingDriveEnergy() {
  using namespace tfdsp::percussion;
  const auto measure = [](const CrashModalField::Parameters &modes) {
    std::pair<std::size_t, double> result{};
    for (const auto &mode : modes) {
      if (mode.inputGain == 0.f || mode.outputGain == 0.f)
        continue;
      ++result.first;
      result.second += static_cast<double>(mode.inputGain) * mode.inputGain;
    }
    return result;
  };
  CrashCymbalFitParameters coherentFit;
  coherentFit.fieldTurbulence = 0.f;
  auto diffuseFit = coherentFit;
  diffuseFit.fieldTurbulence = 1.f;
  const auto coherent = DefaultCrashCymbalParameters(48000.f, coherentFit);
  const auto diffuse = DefaultCrashCymbalParameters(48000.f, diffuseFit);
  const auto coherentMeasure = measure(coherent.modalField);
  const auto diffuseMeasure = measure(diffuse.modalField);
  Check(coherentMeasure.first == 24,
        "zero turbulence leaves exactly one coherent mode per anchor");
  Check(diffuseMeasure.first > coherentMeasure.first &&
            diffuseMeasure.first <= CrashModalFieldModeCount,
        "turbulence allocates sidebands within the shared state pool");
  Check(std::abs(coherentMeasure.second - diffuseMeasure.second) < 1.e-6,
        "modal packet expansion preserves normalized drive energy");

  auto selectiveFit = diffuseFit;
  selectiveFit.fieldTurbulenceScale[0] = 0.f;
  const auto selective = DefaultCrashCymbalParameters(48000.f, selectiveFit);
  const auto selectiveMeasure = measure(selective.modalField);
  const auto firstPacketModes =
      std::count_if(selective.modalField.begin(), selective.modalField.end(),
                    [](const auto &mode) {
                      return mode.inputGain != 0.f && mode.packet == 0;
                    });
  Check(firstPacketModes == 1 && selectiveMeasure.first == diffuseMeasure.first,
        "a clean anchor keeps one centre and releases satellites to the shared "
        "pool");
  Check(
      std::abs(selectiveMeasure.second - diffuseMeasure.second) < 1.e-6,
      "per-anchor turbulence changes coherence without changing drive energy");
}

void TestPairedRingIsNormalizedAndKeepsItsPitch() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.sparseAmplitude.fill(0.f);
  fit.sparseAmplitude[0] = 1.f;
  fit.sparseFrequencyHz[0] = 130.f;
  fit.fieldTurbulence = 0.f;
  fit.fieldDistribution = ModalPacketDistribution::PairedRing;
  fit.fieldDoubletSplitHz = 1.25f;
  fit.fieldBeatRateTilt = 0.f; // isolate the constant-gap case
  fit.bodyDecaySeconds.fill(3.f);
  const auto paired = DefaultCrashCymbalParameters(48000.f, fit).modalField;
  const auto &low = paired[0], &high = paired[1];
  CheckNear(high.frequencyHz - low.frequencyHz, 1.25, 1.e-5,
            "paired ring's audible beat rate is the requested frequency gap");
  CheckNear(.5f * (low.frequencyHz + high.frequencyHz), 130, 1.e-5,
            "paired ring leaves the painted pitch at its midpoint");
  CheckNear(low.inputGain * low.inputGain + high.inputGain * high.inputGain, 1,
            1.e-6, "paired ring preserves unit excitation energy");
  CheckNear(low.transportFrequencyHz, high.transportFrequencyHz, 1.e-6,
            "both partners stay in the same energy-transport cell");
  fit.fieldDoubletSplitHz = 0.f;
  const auto single = DefaultCrashCymbalParameters(48000.f, fit).modalField;
  Check(single[1].inputGain == 0.f, "zero beat rate restores one centre");
  CheckNear(single[0].inputGain,
            low.inputGain * std::cos(low.inputPhaseRadians) +
                high.inputGain * std::cos(high.inputPhaseRadians),
            1.e-6, "splitting a ring does not boost initial observed input");
  CheckNear(low.inputGain * std::sin(low.inputPhaseRadians) +
                high.inputGain * std::sin(high.inputPhaseRadians),
            0, 1.e-6, "quadrature launch cancels at the zero-separation limit");
  for (const float centre : {1.f, 23040.f}) {
    fit.sparseFrequencyHz[0] = centre;
    fit.fieldDoubletSplitHz = 80.f;
    const auto boundary = DefaultCrashCymbalParameters(48000.f, fit).modalField;
    Check(boundary[0].frequencyHz >= 1.f && boundary[1].frequencyHz <= 23040.f,
          "paired frequencies remain inside the represented bandwidth");
  }
}

void TestPairedRingMatchesAnalyticAudio() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.sparseAmplitude.fill(0.f);
  fit.sparseAmplitude[0] = 1.f;
  fit.sparseFrequencyHz[0] = 130.f;
  fit.fieldTurbulence = 0.f;
  fit.fieldDistribution = ModalPacketDistribution::PairedRing;
  fit.fieldDoubletSplitHz = 1.25f;
  fit.bodyDecaySeconds.fill(3.f);
  const auto modes = DefaultCrashCymbalParameters(48000.f, fit).modalField;
  CrashModalField field;
  field.Prepare(48000.f, modes, {}, 700.f, 1500.f);
  double error = 0.0, energy = 0.0;
  constexpr double TwoPi = 6.2831853071795864769;
  for (int sample = 0; sample < 24000; ++sample) {
    const double actual =
        field.ProcessExcitedPair(sample == 0 ? 1.f : 0.f, 0.f);
    const double t = sample / 48000.0;
    double expected = 0;
    for (int i = 0; i < 2; ++i)
      expected += modes[i].inputGain *
                  std::cos(TwoPi * modes[i].frequencyHz * t +
                           modes[i].inputPhaseRadians) *
                  std::pow(.001, t / modes[i].decaySeconds);
    error += (actual - expected) * (actual - expected);
    energy += expected * expected;
  }
  CheckNear(error / energy, 0, 1.e-6,
            "paired ring audio follows its two declared damped tones");
}

void TestPairedRingDepthAndRateTilt() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.sparseAmplitude.fill(0.f);
  fit.sparseAmplitude[0] = 1.f;
  fit.sparseFrequencyHz[0] = 125.f;
  fit.fieldTurbulence = 0.f;
  fit.fieldDistribution = ModalPacketDistribution::PairedRing;
  for (const float depth : {0.f, .1f, .2f, .5f, 1.f}) {
    fit.fieldBeatDepth = depth;
    const auto modes = DefaultCrashCymbalParameters(48000.f, fit).modalField;
    const auto &a = modes[0], &b = modes[1];
    CheckNear(a.inputGain * a.inputGain + b.inputGain * b.inputGain, 1, 1.e-6,
              "beat depth preserves input energy");
    CheckNear(a.inputGain * std::cos(a.inputPhaseRadians) +
                  b.inputGain * std::cos(b.inputPhaseRadians),
              1, 1.e-6, "beat depth preserves initial observation");
    CheckNear(a.inputGain * std::sin(a.inputPhaseRadians) +
                  b.inputGain * std::sin(b.inputPhaseRadians),
              0, 1.e-6, "beat depth cancels imaginary launch");
    if (depth > 0.f)
      CheckNear(a.inputGain / b.inputGain, depth, 1.e-6,
                "depth is pair amplitude ratio");
    else
      CheckNear(a.frequencyHz + b.frequencyHz, 250, 1.e-6,
                "zero depth is unsplit");
  }
  CheckNear(RingBeatRate(125, 1.25f, .5f), 1.25, 1.e-6,
            "rate tilt preserves base rate");
  CheckNear(RingBeatRate(500, 1.25f, .5f), 2.5, 1.e-6,
            "rate tilt scales by octaves");
  CheckNear(RingBeatRate(15000, 80, 1), 80, 1.e-6, "rate tilt remains bounded");
}

void TestPaintedLevelsOnlyShapeNormalizedObservation() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters firstFit;
  firstFit.sparseAmplitude.fill(0.f);
  firstFit.sparseFrequencyHz[0] = 400.f;
  firstFit.sparseFrequencyHz[1] = 3200.f;
  firstFit.sparseAmplitude[0] = 1.f;
  firstFit.sparseAmplitude[1] = .25f;
  firstFit.fieldTurbulence = 0.f;
  firstFit.bodyTiltDbPerOctave = 0.f;
  auto secondFit = firstFit;
  secondFit.sparseAmplitude[0] = .25f;
  secondFit.sparseAmplitude[1] = 1.f;
  const auto first = DefaultCrashCymbalParameters(48000.f, firstFit);
  const auto second = DefaultCrashCymbalParameters(48000.f, secondFit);
  double firstOutputEnergy = 0.0;
  double secondOutputEnergy = 0.0;
  for (std::size_t mode = 0; mode < 2; ++mode) {
    CheckNear(first.modalField[mode].inputGain,
              second.modalField[mode].inputGain, 1.e-7,
              "painted prominence does not alter strike-energy allocation");
    firstOutputEnergy +=
        first.modalField[mode].outputGain * first.modalField[mode].outputGain;
    secondOutputEnergy +=
        second.modalField[mode].outputGain * second.modalField[mode].outputGain;
  }
  Check(first.modalField[0].outputGain > first.modalField[1].outputGain &&
            second.modalField[0].outputGain < second.modalField[1].outputGain,
        "painted prominence controls modal observation balance");
  CheckNear(firstOutputEnergy, 1.0625, 2.e-7,
            "painted observation uses the exact bar amplitudes");
  CheckNear(secondOutputEnergy, 1.0625, 2.e-7,
            "swapping bars preserves their squared observation weights");
}

void TestExcitationShelfCentreChangesNormalizedColour() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.sparseAmplitude.fill(0.f);
  fit.sparseFrequencyHz[0] = 400.f;
  fit.sparseFrequencyHz[1] = 3200.f;
  fit.sparseAmplitude[0] = 1.f;
  fit.sparseAmplitude[1] = 1.f;
  fit.fieldTurbulence = 0.f;
  fit.bodyTiltDbPerOctave = -12.f;
  fit.bodyExcitationCentreHz = 400.f;
  const auto lowCentre = DefaultCrashCymbalParameters(48000.f, fit);
  fit.bodyExcitationCentreHz = 1600.f;
  const auto highCentre = DefaultCrashCymbalParameters(48000.f, fit);
  const float lowCentreRatio =
      lowCentre.modalField[0].inputGain / lowCentre.modalField[1].inputGain;
  const float highCentreRatio =
      highCentre.modalField[0].inputGain / highCentre.modalField[1].inputGain;
  Check(lowCentreRatio > 2.f * highCentreRatio,
        "excitation centre moves the normalized shelf knee");
  for (const auto &parameters : {lowCentre, highCentre}) {
    double energy = 0.0;
    for (std::size_t mode = 0; mode < 2; ++mode)
      energy += parameters.modalField[mode].inputGain *
                parameters.modalField[mode].inputGain;
    CheckNear(energy, 1.0, 2.e-7,
              "excitation shelf centre preserves drive energy");
  }
}

void TestUnifiedFieldAcceptsConstructiveAnchorEditing() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.sparseAmplitude.fill(0.f);
  fit.sparseFrequencyHz[0] = 7300.f;
  fit.sparseFrequencyHz[1] = 20.f;
  fit.sparseAmplitude[0] = .75f;
  fit.sparseAmplitude[1] = .25f;
  fit.fieldTurbulenceScale.fill(0.f);
  fit.fieldTurbulenceScale[1] = 1.f;
  const auto parameters = DefaultCrashCymbalParameters(48000.f, fit);
  const auto activeInPacket = [&](const std::uint16_t packet) {
    return std::count_if(
        parameters.modalField.begin(), parameters.modalField.end(),
        [packet](const auto &mode) {
          return mode.packet == packet && mode.inputGain != 0.f;
        });
  };
  Check(activeInPacket(0) > 1 && activeInPacket(1) == 1,
        "frequency sorting keeps each edited anchor's turbulence attached");

  fit.sparseAmplitude.fill(0.f);
  const auto cleared = DefaultCrashCymbalParameters(48000.f, fit);
  Check(std::none_of(cleared.modalField.begin(), cleared.modalField.end(),
                     [](const auto &mode) { return mode.inputGain != 0.f; }),
        "zero anchor energy silences the unified modal field");
}

} // namespace crash_test
