#include "crash_test_support.hpp"

namespace crash_test {

void TestBodyDecayCurveUsesActiveErbKnots() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  Check(std::count(fit.bodyDecayActive.begin(), fit.bodyDecayActive.end(),
                   true) == 0,
        "crash T60 defaults to no optional interior knots");
  fit.bodyDecayActive.fill(false);
  fit.bodyDecaySeconds.fill(1.f);
  fit.bodyDecayFrequencyHz[0] = 1000.f;
  fit.bodyDecaySeconds[1] = 8.f;
  fit.bodyDecayActive[0] = true;
  fit.sparseFrequencyHz[0] = 1000.f;
  const auto withKnot = DefaultCrashCymbalParameters(48000.f, fit);
  const auto nearestDecay = [](const auto &parameters) {
    const auto mode = std::min_element(
        parameters.modalField.begin(), parameters.modalField.end(),
        [](const auto &left, const auto &right) {
          return std::abs(left.frequencyHz - 1000.f) <
                 std::abs(right.frequencyHz - 1000.f);
        });
    return mode->decaySeconds;
  };
  Check(std::abs(nearestDecay(withKnot) - 8.f) < 1.e-4f,
        "active body T60 knots are sampled by modal preparation");

  fit.bodyDecayActive[0] = false;
  const auto withoutKnot = DefaultCrashCymbalParameters(48000.f, fit);
  Check(std::abs(nearestDecay(withoutKnot) - 1.f) < 1.e-4f,
        "inactive body T60 knots do not affect modal decay");
}

void TestFiniteAtSupportedRates() {
  for (const float sampleRate : {44100.f, 48000.f, 96000.f, 192000.f}) {
    const auto audio = Render(1.f, .73f, 1.f, 0xffffffffu, .5f, sampleRate);
    Check(std::all_of(audio.begin(), audio.end(),
                      [](const float sample) {
                        return std::isfinite(sample) &&
                               std::abs(sample) < 100.f;
                      }),
          "crash remains finite and bounded across sample rates");
  }
}

void TestAnalysisFrameMatchesOutput() {
  using namespace tfdsp::percussion;
  CrashCymbal framed;
  CrashCymbal plain;
  const auto parameters = DefaultCrashCymbalParameters(48000.f);
  framed.Prepare(48000.f, parameters);
  plain.Prepare(48000.f, parameters);
  framed.Trigger({.8f, 1.f, .65f, 71});
  plain.Trigger({.8f, 1.f, .65f, 71});
  for (int sample = 0; sample < 4096; ++sample) {
    const auto frame = framed.ProcessFrame();
    Check(frame.output == plain.Process(),
          "crash analysis taps do not change production output");
    Check(std::isfinite(frame.directContact) &&
              std::isfinite(frame.bloomTransferEnergy) &&
              std::isfinite(frame.modalBody),
          "crash analysis taps remain finite");
  }
}

void TestSharedOutputEq() {
  using namespace tfdsp::percussion;
  for (const float rate : {44100.f, 48000.f, 96000.f}) {
    for (const bool enabled : {false, true}) {
      CrashCymbalFitParameters fit;
      fit.directGain = .37f;
      fit.fieldGain = .81f;
      fit.outputGain = .7f;
      fit.outputEqEnabled = enabled;
      fit.outputLowCutHz = 250.f;
      fit.outputColourFrequencyHz = 1400.f;
      fit.outputColourGainDb = 6.f;
      fit.outputHighCutHz = 6500.f;
      const auto parameters = DefaultCrashCymbalParameters(rate, fit);
      CrashCymbal cymbal;
      cymbal.Prepare(rate, parameters);
      RadiationFilter expected;
      expected.Prepare(rate, parameters.outputEq);
      for (int pass = 0; pass < 2; ++pass) {
        cymbal.Reset();
        expected.Reset();
        cymbal.Trigger({.8f, .7f, .6f, 81});
        for (int sample = 0; sample < 4000; ++sample) {
          const auto frame = cymbal.ProcessFrame();
          const float mix = fit.directGain * frame.directContact +
                            fit.fieldGain * frame.modalBody;
          const float output =
              fit.outputGain * (enabled ? expected.Process(mix) : mix);
          CheckNear(frame.output, output, 1.e-7,
                    "one final EQ processes the full mix, with exact bypass "
                    "and reset");
        }
      }
    }
  }
}

void TestUpperModalRangeAndDecay() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.sparseAmplitude.fill(0.f);
  fit.sparseAmplitude[0] = 1.f;
  fit.sparseFrequencyHz[0] = 19000.f;
  fit.fieldTurbulence = 0.f;
  fit.bodyDecayActive.fill(false);
  fit.bodyDecaySeconds.front() = 8.f;
  fit.bodyDecaySeconds.back() = 2.f;
  const auto legacy = DefaultCrashCymbalParameters(48000.f, fit);
  CheckNear(legacy.modalField[0].frequencyHz, 19000, .01,
            "upper modal centre is not clipped at 15 kHz");
  CheckNear(legacy.modalField[0].decaySeconds, 2, 1.e-5,
            "legacy decay extends flat above its 15 kHz endpoint");
  fit.bodyDecayMaximumFrequencyHz = 20000.f;
  for (float rate : {32000.f, 44100.f, 48000.f, 96000.f}) {
    const auto p = DefaultCrashCymbalParameters(rate, fit);
    Check(p.modalField[0].frequencyHz <= .48f * rate,
          "extended modes respect sample-rate guard");
    if (rate >= 44100)
      Check(p.modalField[0].decaySeconds > 2.f &&
                p.modalField[0].decaySeconds < 2.1f,
            "extended endpoint controls decay above 15 kHz");
    CrashCymbal voice;
    voice.Prepare(rate, p);
    for (int hit = 0; hit < 3; ++hit) {
      voice.Trigger({1.f, .5f, .65f, 91});
      for (int sample = 0; sample < 2000; ++sample)
        Check(std::isfinite(voice.Process()),
              "upper-range restrikes remain finite");
    }
  }
}

} // namespace crash_test
