#include "membrane_test_support.hpp"

namespace membrane_test {

void TestAcousticKickEnergyBudget() {
  using namespace tfdsp::percussion;
  MembraneDrumControls controls;
  controls.fundamentalHz = 35.f;
  controls.decaySeconds = .25f;
  controls.decayTilt = .7f;
  controls.inharmonicity = .18f;
  controls.bodyBrightness = .28f;
  controls.tensionOctaves = .1f;
  controls.tensionDecaySeconds = .05f;
  controls.contactDirectLevel = .287f;
  controls.contactBodyLevel = .205f;
  controls.contactDurationSeconds = .0065f;
  controls.contactBrightness = .38f;
  controls.fmDirectLevel = .04f;
  controls.fmBodyLevel = .05625f;
  controls.fmDepthHz = 520.f;
  controls.fmDecaySeconds = .07f;
  controls.pitchDropOctaves = 1.f;
  auto parameters = DefaultMembraneDrumParameters(controls);
  MembraneDrum drum;
  drum.Prepare(48000.f, parameters);
  drum.Trigger({1.f, .5f, .5f, .5f, .2f, 37});
  float maximumEnergy = 0.f;
  for (std::size_t sample = 0; sample < 48000; ++sample) {
    drum.Process();
    maximumEnergy = std::max(maximumEnergy, drum.ModalEnergy());
  }
  Check(maximumEnergy > 0 && maximumEnergy < 4,
        "normalized acoustic-kick contact needs no energy ceiling");
}

void TestDefaultHeadroom() {
  using namespace tfdsp::percussion;
  const auto parameters = DefaultMembraneDrumParameters();
  for (const float strength : {.2f, .5f, .8f, 1.f}) {
    for (const float location : {0.f, .5f, 1.f}) {
      for (const float implement : {0.f, .5f, 1.f}) {
        const auto audio = Render(
            48000.f, {strength, location, .8f, implement, .5f, 73}, parameters);
        const auto peak =
            *std::max_element(audio.begin(), audio.end(),
                              [](const float left, const float right) {
                                return std::abs(left) < std::abs(right);
                              });
        if (!(std::abs(peak) < 1.f))
          std::cerr << "membrane single-hit peak " << strength << '/'
                    << location << '/' << implement << ": " << std::abs(peak)
                    << '\n';
        Check(std::abs(peak) < 1.f,
              "default single hits retain normalized-output headroom");
      }
    }
  }
  for (const float sampleRate : {44100.f, 48000.f, 96000.f, 192000.f}) {
    MembraneDrum drum;
    drum.Prepare(sampleRate, parameters);
    float peak = 0.f;
    float maximumEnergy = 0.f;
    const auto frames = static_cast<std::size_t>(2.f * sampleRate);
    const auto retrigger = static_cast<std::size_t>(.0625f * sampleRate);
    for (std::size_t sample = 0; sample < frames; ++sample) {
      if (sample % retrigger == 0)
        drum.Trigger(
            {1.f, .5f, .8f, 1.f, .2f, static_cast<std::uint32_t>(sample + 1)});
      peak = std::max(peak, std::abs(drum.Process()));
      maximumEnergy = std::max(maximumEnergy, drum.ModalEnergy());
    }
    if (!(peak > .05f && peak < 1.f))
      std::cerr << "membrane retrigger peak/energy at " << sampleRate << ": "
                << peak << '/' << maximumEnergy << '\n';
    Check(std::isfinite(maximumEnergy) && maximumEnergy < 32,
          "normalized repeated contacts retain useful energy headroom");
    Check(peak > .05f && peak < 1.f,
          "default membrane stays audible with normalized-output headroom");
  }
}

void TestIndependentFmPitchTime() {
  using namespace tfdsp::percussion;
  MembraneDrumControls controls;
  controls.fmDecaySeconds = .4f;
  controls.fmPitchDecaySeconds = .02f;
  const auto first = DefaultMembraneDrumParameters(controls);
  CheckNear(first.fm.carrierFrequencyHz.segments[0].durationSeconds, .02, 1.e-6,
            "FM pitch fall has an independent visible duration");
  CheckNear(first.fm.amplitude.segments[1].durationSeconds, .4, 1.e-6,
            "short pitch fall does not shorten FM amplitude decay");
  controls.fmDecaySeconds = .15f;
  const auto second = DefaultMembraneDrumParameters(controls);
  CheckNear(second.fm.carrierFrequencyHz.segments[0].durationSeconds, .02,
            1.e-6, "amplitude edits cannot silently alter pitch timing");
}

void TestFmDepthActuallyDecaysGeometrically() {
  using namespace tfdsp::percussion;
  MembraneDrumControls controls;
  controls.fmDepthHz = 500.f;
  controls.fmDecaySeconds = .2f;
  const auto parameters = DefaultMembraneDrumParameters(controls);
  BreakpointTrajectory<CorrelatedFmMaximumSegments> depth;
  depth.Prepare(48000.f);
  depth.Start(parameters.fm.frequencyDeviationHz.initialValue,
              parameters.fm.frequencyDeviationHz.segments,
              parameters.fm.frequencyDeviationHz.segmentCount);
  for (int i = 0; i < 3840; ++i)
    depth.Process();
  CheckNear(depth.Value(), 5., .01,
            "halfway through an 80 dB geometric FM fade is -40 dB, not half "
            "amplitude");
  for (int i = 0; i < 4000; ++i)
    depth.Process();
  CheckNear(depth.Value(), 0., 1.e-8, "FM fade finishes at exact zero");
}

void TestIndependentContactNoise() {
  using namespace tfdsp::percussion;
  MembraneDrumControls controls;
  controls.contactNoiseLevel = 1.3f;
  controls.contactNoiseDecaySeconds = .2f;
  const auto parameters = DefaultMembraneDrumParameters(controls);
  CheckNear(parameters.contact.noise.amplitude, 1.3, 1.e-6,
            "contact noise level is an explicit control");
  CheckNear(parameters.contact.noise.decaySeconds, .2, 1.e-6,
            "contact noise has an independent fade time");
  CheckNear(parameters.contact.pulseDurationSeconds,
            controls.contactDurationSeconds, 1.e-6,
            "longer noise does not stretch the contact pulse");
  CheckNear(parameters.membrane[0].decaySeconds, controls.decaySeconds, 1.e-6,
            "noise duration does not change membrane damping");
}

} // namespace membrane_test
