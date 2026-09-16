#include "membrane_test_support.hpp"

namespace membrane_test {

void TestKickObservationPaths() {
  using namespace tfdsp::percussion;
  {
    ContactExciter full, noise;
    full.Prepare(48000.f);
    noise.Prepare(48000.f);
    auto contact = DefaultKickVoiceParameters().contact;
    contact.noise.amplitude = 0.f;
    full.Trigger(contact);
    noise.Trigger(contact);
    double difference = 0.;
    for (int i = 0; i < 2400; ++i) {
      const auto a = full.Process(), b = noise.Process(true);
      CheckNear(a.bodyDrive, b.bodyDrive, 0.,
                "noise observation selection leaves body excitation identical");
      CheckNear(b.directRadiation, 0., 0.,
                "noise-only observation excludes pulse/chirp/grains");
      difference += std::abs(a.directRadiation - b.directRadiation);
    }
    Check(difference > .1,
          "observation selection actually changes the direct signal");
    EnvelopedNoiseBurst expected;
    expected.Prepare(48000.f);
    contact.noise.amplitude = .7f;
    noise.Reset();
    noise.Trigger(contact);
    expected.Trigger(contact.noise);
    double noiseEnergy = 0.;
    for (int i = 0; i < 2400; ++i) {
      const float sample = noise.Process(true).directRadiation;
      CheckNear(sample, expected.Process(), 0.,
                "noise observation is the exact noise primitive");
      noiseEnergy += sample * sample;
    }
    Check(noiseEnergy > .01,
          "noise observation remains audible when noise is enabled");
    noise.Reset();
    noise.Trigger(contact);
    double laterNoise = 0.;
    for (int i = 0; i < 2400; ++i) {
      const auto sample = noise.Process(true, true);
      if (i > static_cast<int>(48000.f * contact.pulseDurationSeconds) + 2) {
        CheckNear(
            sample.bodyDrive, 0., 0.,
            "pulse-only drive ends independently of the observed noise tail");
        laterNoise += sample.directRadiation * sample.directRadiation;
      }
    }
    Check(laterNoise > .001,
          "direct noise can continue after body excitation stops");
  }
}

void TestKickDecayShapes() {
  using namespace tfdsp::percussion;
  KickVoiceControls controls;
  for (float rate : {44100.f, 48000.f, 96000.f}) {
    for (float shape : {0.f, .5f, 1.f}) {
      controls.thumpDecaySeconds = .2f;
      controls.thumpDecayShape = shape;
      const auto shaped = DefaultKickVoiceParameters(controls);
      BreakpointTrajectory<CorrelatedFmMaximumSegments> envelope;
      envelope.Prepare(rate);
      envelope.Start(shaped.fm.amplitude.initialValue,
                     shaped.fm.amplitude.segments,
                     shaped.fm.amplitude.segmentCount);
      float previous = 1.f;
      const int attack = static_cast<int>(std::lround(.0004f * rate));
      for (int i = 0; i < attack + static_cast<int>(std::lround(.2f * rate));
           ++i) {
        const float value = envelope.Process();
        if (i >= attack) {
          Check(value <= previous + 1.e-6f,
                "shaped thump never adds decay energy");
          previous = value;
        }
      }
      CheckNear(envelope.Value(), .001, 3.e-6,
                "curved thump preserves T60 across shapes and sample rates");
    }
  }
}

void TestUnifiedKickSurface() {
  using namespace tfdsp::percussion;
  TestKickObservationPaths();
  TestKickDecayShapes();
  KickVoiceControls controls;
  controls.thumpHoldSeconds = .025f;
  const auto held = DefaultKickVoiceParameters(controls);
  Check(held.fm.amplitude.segmentCount == 4, "hold adds one source segment");
  CheckNear(held.fm.amplitude.segments[1].targetValue, 1., 1.e-6,
            "hold keeps full amplitude without clipping the waveform");
  CheckNear(held.fm.amplitude.segments[1].durationSeconds, .025, 1.e-6,
            "hold duration follows the explicit control");
  for (const auto &mode : held.membrane) {
    CheckNear(mode.centerProjection, 1., 1.e-6, "fixed kick projection");
    CheckNear(mode.edgeProjection, 1., 1.e-6,
              "kick projection is position independent");
  }
  controls.thumpHoldSeconds = 0.f;
  controls.thumpPitchHz = 20.f;
  controls.thumpDecaySeconds = .3f;
  controls.modes[0].frequencyHz = 40.f;
  controls.resonanceLevel = 0.f;
  controls.contactLevel = 0.f;
  controls.contactNoiseDecaySeconds = .15f;
  const auto parameters = DefaultKickVoiceParameters(controls);
  CheckNear(parameters.fm.carrierFrequencyHz.segments[0].targetValue, 20.,
            1.e-6,
            "kick thump can settle below the membrane recipe's 25 Hz limit");
  CheckNear(parameters.membrane[0].frequencyHz, 40., 1.e-6,
            "mode frequency is independent of thump pitch");
  controls.modes[0].frequencyHz = 200.f;
  controls.resonanceDecaySeconds = 1.f;
  controls.resonanceDecayTilt = 1.f;
  auto frequencyLoss = DefaultKickVoiceParameters(controls);
  CheckNear(frequencyLoss.membrane[0].decaySeconds, .5, 1.e-6,
            "global frequency loss halves T60 one octave above 100 Hz");
  controls.modes[1].levelDb = -72.f;
  frequencyLoss = DefaultKickVoiceParameters(controls);
  CheckNear(frequencyLoss.membrane[0].decaySeconds, .5, 1.e-6,
            "disabling another mode cannot change damping");
  CheckNear(frequencyLoss.membrane[1].inputGain, 0., 1.e-6,
            "disabled modes receive no strike energy");
  CheckNear(frequencyLoss.membrane[1].outputGain, 0., 1.e-6,
            "disabled modes have no observation");
  CheckNear(parameters.fm.amplitude.segments[1].durationSeconds, .4, 1.e-6,
            "kick source T60 converts correctly to its finite -80 dB fade");
  CheckNear(parameters.contact.noise.decaySeconds, .2, 1.e-6,
            "contact noise T60 is not confused with fade duration");
  CheckNear(parameters.contactBodyLevel, 1., 1.e-6,
            "muting audible contact does not mute its unit body excitation");
  CheckNear(parameters.fmBodyLevel, 0., 1.e-6,
            "thump does not drive resonance");
  CheckNear(parameters.fm.frequencyDeviationHz.initialValue, 0., 1.e-6,
            "thump is a clean oscillator, not a forced FM layer");
  auto muted = parameters;
  ApplyKickRouting(muted, {{false, false, false}});
  Check(muted.routing.Enabled(MembraneDrumRoute::ContactToBody),
        "muting observations retains the required contact-to-resonator path");
  for (unsigned mask = 0; mask < 8; ++mask) {
    KickVoiceRouting routing;
    for (unsigned i = 0; i < 3; ++i)
      routing.enabled[i] = mask & (1u << i);
    auto routed = DefaultKickVoiceParameters();
    ApplyKickRouting(routed, routing);
    const auto audio = Render(48000.f, {.5f, .5f, .5f, .5f, .2f, 37}, routed);
    Check(std::all_of(audio.begin(), audio.end(),
                      [](float value) { return std::isfinite(value); }),
          "every kick routing mask remains finite");
    Check((Energy(audio) > 1.e-10) == (mask != 0),
          "each kick branch is independently audible, all off is silent");
  }
}

void TestKickQuietModeRemovalIsContinuous() {
  using namespace tfdsp::percussion;
  KickVoiceControls controls;
  controls.contactLevel = controls.thumpLevel = controls.tensionOctaves = 0.f;
  for (auto &mode : controls.modes)
    mode = {110.f, -72.f};
  controls.modes[0] = {55.f, 0.f};
  controls.modes[1].levelDb = -71.99f;
  const auto near = Render(48000.f, {.5f, 0.f, .5f, .5f, .2f, 1449},
                           DefaultKickVoiceParameters(controls));
  controls.modes[1].levelDb = -72.f;
  const auto off = Render(48000.f, {.5f, 0.f, .5f, .5f, .2f, 1449},
                          DefaultKickVoiceParameters(controls));
  Check(std::abs(10. * std::log10(Energy(off) / Energy(near))) < .001,
        "removing a -71.99 dB mode cannot boost the remaining modes");
  double error = 0.;
  for (std::size_t i = 0; i < off.size(); ++i)
    error += (off[i] - near[i]) * (off[i] - near[i]);
  Check(error / Energy(off) < 1.e-8, "quiet-mode off boundary is continuous");
}

} // namespace membrane_test
