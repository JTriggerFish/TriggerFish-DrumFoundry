#include "crash_test_support.hpp"

namespace crash_test {

void TestDeterministicAndResponsive() {
  const auto first = Render(.8f, 1.f, .65f, 1234);
  const auto repeated = Render(.8f, 1.f, .65f, 1234);
  const auto differentSeed = Render(.8f, 1.f, .65f, 1235);
  Check(first == repeated, "crash rendering is deterministic for one seed");
  Check(Difference(first, differentSeed) > 1.e-7,
        "crash contact variation responds to its seed");

  const auto bell = Render(.8f, 0.f, .65f, 1234);
  const auto bow = Render(.8f, .5f, .65f, 1234);
  Check(Difference(bell, bow) > .01 * Energy(bow),
        "crash location changes the body projection audibly");

  const auto soft = Render(.8f, 1.f, .15f, 1234);
  const auto hard = Render(.8f, 1.f, .95f, 1234);
  Check(Difference(soft, hard) > .01 * Energy(hard),
        "crash hardness changes the contact audibly");
}

void TestVelocityEnergy() {
  const std::array<float, 4> strengths{.2f, .45f, .7f, 1.f};
  double previous = 0.0;
  for (const float strength : strengths) {
    const double current = Energy(Render(strength, 1.f, .65f, 81));
    if (!(current > previous))
      std::cerr << "crash velocity energy " << strength << ": " << current
                << " after " << previous << '\n';
    Check(current > previous,
          "crash energy increases across the velocity sweep");
    previous = current;
  }
}

void TestVelocityChangesCymbalRegime() {
  using namespace tfdsp::percussion;
  constexpr float sampleRate = 48000.f;
  const std::array<float, 4> strengths{.2f, .45f, .7f, 1.f};
  std::array<double, strengths.size()> brightness{};
  for (std::size_t index = 0; index < strengths.size(); ++index)
    brightness[index] = NormalizedDifferenceEnergy(
        Render(strengths[index], 1.f, .65f, 81, .75f));
  bool monotonicBrightness = true;
  for (std::size_t index = 1; index < brightness.size(); ++index)
    monotonicBrightness =
        monotonicBrightness && brightness[index] > brightness[index - 1];
  if (!monotonicBrightness) {
    std::cerr << "crash velocity brightness:";
    for (const double value : brightness)
      std::cerr << ' ' << value;
    std::cerr << '\n';
  }
  Check(monotonicBrightness && brightness.back() > 1.05 * brightness.front(),
        "each stronger crash strike excites a brighter spectrum");

  struct BranchEnergy {
    double direct{};
    double bloom{};
    double dense{};
  };
  const auto measure = [](const float strength,
                          const float energyAcceleration = .7f) {
    CrashCymbal cymbal;
    CrashCymbalFitParameters fit;
    fit.bloomEnergyAcceleration = energyAcceleration;
    cymbal.Prepare(sampleRate, DefaultCrashCymbalParameters(sampleRate, fit));
    cymbal.Trigger({strength, 1.f, .65f, 81});
    BranchEnergy result;
    for (int sample = 0; sample < 24000; ++sample) {
      const auto frame = cymbal.ProcessFrame();
      result.direct +=
          static_cast<double>(frame.directContact) * frame.directContact;
      result.bloom += frame.bloomTransferEnergy;
      result.dense += static_cast<double>(frame.modalBody) * frame.modalBody;
    }
    return result;
  };
  const auto quietBranches = measure(.25f);
  const auto loudBranches = measure(1.f);
  const double directGrowth = loudBranches.direct / quietBranches.direct;
  const double bloomGrowth = loudBranches.bloom / quietBranches.bloom;
  const double denseGrowth = loudBranches.dense / quietBranches.dense;
  const auto quietIndependent = measure(.25f, 0.f);
  const auto loudIndependent = measure(1.f, 0.f);
  const double independentBloomGrowth =
      loudIndependent.bloom / quietIndependent.bloom;
  if (!(bloomGrowth > 1.01 * independentBloomGrowth && denseGrowth > 1.0)) {
    std::cerr << "crash velocity direct/bloom/dense growth: " << directGrowth
              << '/' << bloomGrowth << '/' << denseGrowth
              << "; independent bloom " << independentBloomGrowth << '\n';
  }
  Check(bloomGrowth > 1.01 * independentBloomGrowth && denseGrowth > 1.0,
        "strong strikes accelerate transfer without relying on an unnormalized "
        "brightness gain");
}

void TestImplementFamiliesAreDistinct() {
  using namespace tfdsp::percussion;
  constexpr float sampleRate = 48000.f;
  const auto render = [](const float implement, const float spread = .2f) {
    CrashCymbal cymbal;
    cymbal.Prepare(sampleRate, DefaultCrashCymbalParameters(sampleRate));
    cymbal.Trigger({.8f, .75f, .65f, 91, implement, spread});
    std::vector<float> result(12000);
    for (float &sample : result)
      sample = cymbal.Process();
    return result;
  };
  const auto brush = render(0.f);
  const auto mallet = render(.5f);
  const auto stick = render(1.f);
  const auto brushSweep = render(0.f, 1.f);
  const double brushToStickEnergy =
      Energy(brush) / std::max(Energy(stick), 1.e-30);
  Check(Difference(brush, mallet) > 1.e-5 * Energy(mallet),
        "brush and mallet contacts are distinct");
  Check(Difference(mallet, stick) > 1.e-5 * Energy(stick),
        "mallet and stick contacts are distinct");
  Check(Difference(brush, brushSweep) > .01 * Energy(brushSweep),
        "brush contact spread changes a tap into a sustained gesture");
  // The generic core preset is deliberately not implement-level matched; the
  // workbench preset owns that perceptual calibration and tests it separately.
  if (!(brushToStickEnergy > .001 && brushToStickEnergy < 4.0))
    std::cerr << "crash brush/stick energy ratio: " << brushToStickEnergy
              << '\n';
  Check(brushToStickEnergy > .001 && brushToStickEnergy < 4.0,
        "generic brush output remains audible without level matching");

  const auto contactShape = [](const float implement) {
    CrashCymbal cymbal;
    cymbal.Prepare(sampleRate, DefaultCrashCymbalParameters(sampleRate));
    cymbal.Trigger({.8f, .75f, .65f, 91, implement, .2f});
    std::array<double, 2> energy{};
    for (int sample = 0; sample < 12000; ++sample) {
      const double contact = cymbal.ProcessFrame().directContact;
      energy[sample < 96 ? 0 : 1] += contact * contact;
    }
    return energy;
  };
  const auto brushContact = contactShape(0.f);
  const auto stickContact = contactShape(1.f);
  const double brushTailRatio =
      brushContact[1] / std::max(brushContact[0], 1.e-30);
  const double stickTailRatio =
      stickContact[1] / std::max(stickContact[0], 1.e-30);
  Check(brushTailRatio > 4.0 * stickTailRatio,
        "brush contact is distributed instead of becoming a stick transient");
}

void TestDefaultBodyCoversTheMeasuredLowRegion() {
  using namespace tfdsp::percussion;
  const auto parameters = DefaultCrashCymbalParameters(48000.f);
  const auto lowModes =
      std::count_if(parameters.modalField.begin(), parameters.modalField.end(),
                    [](const auto &mode) { return mode.frequencyHz < 500.f; });
  Check(lowModes >= 1,
        "default crash retains a resolved plate ridge below 500 Hz");
  Check(parameters.outputEq.lowCutHz <= 50.f,
        "default crash observation does not remove its low plate body");
}

void TestContactCalibrationMacrosAreAudible() {
  using namespace tfdsp::percussion;
  constexpr float sampleRate = 48000.f;
  CrashCymbalFitParameters firstFit;
  CrashCymbalFitParameters secondFit;
  secondFit.contactNoiseGain = 2.f;
  secondFit.contactChirpFrequencyScale = 1.5f;
  CrashCymbal first;
  CrashCymbal second;
  first.Prepare(sampleRate, DefaultCrashCymbalParameters(sampleRate, firstFit));
  second.Prepare(sampleRate,
                 DefaultCrashCymbalParameters(sampleRate, secondFit));
  first.Trigger({.8f, 1.f, .65f, 1234});
  second.Trigger({.8f, 1.f, .65f, 1234});
  double difference = 0.0;
  for (int sample = 0; sample < 720; ++sample) {
    const double delta = first.Process() - second.Process();
    difference += delta * delta;
  }
  Check(difference > 1.e-8,
        "crash contact calibration macros change the initial transient");
}

} // namespace crash_test
