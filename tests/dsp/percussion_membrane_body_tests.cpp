#include "membrane_test_support.hpp"

namespace membrane_test {

void TestMixerAndStrikeEnergy() {
  tfdsp::percussion::FixedMixer<3> mixer;
  mixer.SetGains({1.f, -2.f, std::numeric_limits<float>::infinity()});
  CheckNear(mixer.Process({.25f, .125f, 100.f}), 0., 1.e-7,
            "fixed mixer applies bounded independent gains");

  tfdsp::percussion::StrikeEnergyEnvelope state;
  state.Prepare(48000.f, {.1f, 2.f, .25f});
  state.InjectStrike(.5f);
  const float first = state.Value();
  for (int sample = 0; sample < 4800; ++sample)
    state.Process();
  Check(state.Value() < first && state.Value() > 0.f,
        "strike-energy envelope decays passively");
  const float once = state.Value();
  state.InjectStrike(.5f);
  Check(state.Value() > once,
        "repeated strikes accumulate in the strike-history envelope");

  tfdsp::percussion::StrikeEnergyEnvelope highRate;
  highRate.Prepare(96000.f, {.1f, 2.f, .25f});
  highRate.InjectStrike(.5f);
  for (int sample = 0; sample < 9600; ++sample)
    highRate.Process();
  CheckNear(highRate.Value(), once, 1.e-4,
            "strike-energy decay is sample-rate invariant");
}

void TestDynamicTension() {
  const auto nominal = ZeroCrossings(1.f);
  const auto raised = ZeroCrossings(1.5f);
  Check(raised > nominal * 1.45 && raised < nominal * 1.55,
        "membrane tension scales modal frequency without retuning decay");
}

void TestEqualizerModes() {
  using namespace tfdsp::percussion;
  ObservationEqualizer equalizer;
  ObservationEqualizerParameters parameters;
  parameters.mode = ObservationEqualizerMode::Bypass;
  parameters.outputGain = .5f;
  equalizer.Prepare(48000.f, parameters);
  CheckNear(equalizer.Process(.25f), .125, 0.,
            "bypass observation EQ is a gain-only wire");
  parameters.mode = ObservationEqualizerMode::Multiband;
  parameters.bands[2].gainDb = 12.f;
  equalizer.Prepare(48000.f, parameters);
  float energy = 0.f;
  for (std::size_t sample = 0; sample < 4096; ++sample) {
    const float output =
        equalizer.Process(percussion_test::Sine(sample, 1800.f, 48000.f));
    energy += output * output;
  }
  Check(std::isfinite(energy) && energy > 100.f,
        "multiband observation EQ is finite and active");
}

void TestMembraneRecipe() {
  using namespace tfdsp::percussion;
  const auto parameters = DefaultMembraneDrumParameters();
  const MembraneDrumHit center{.8f, 0.f, .5f, 1.f, .25f, 17};
  auto first = Render(48000.f, center, parameters);
  auto repeated = Render(48000.f, center, parameters);
  Check(first == repeated, "membrane recipe is deterministic for one seed");
  Check(Energy(first) > 1.e-5,
        "membrane recipe produces audible finite output");
  Check(std::all_of(first.begin(), first.end(),
                    [](const float sample) { return std::isfinite(sample); }),
        "membrane recipe remains finite");

  const auto edge = Render(48000.f, {.8f, 1.f, .5f, 1.f, .25f, 17}, parameters);
  double difference = 0.;
  for (std::size_t index = 0; index < first.size(); ++index) {
    const double delta = first[index] - edge[index];
    difference += delta * delta;
  }
  Check(difference > 1.e-5,
        "strike location changes membrane modal projection");

  MembraneDrum drum;
  drum.Prepare(48000.f, parameters);
  drum.Trigger(center);
  for (int sample = 0; sample < 1000; ++sample)
    drum.Process();
  const float once = drum.StrikeEnergy();
  drum.Trigger(center);
  Check(drum.StrikeEnergy() > once,
        "overlapping hits add to persistent strike history");
  Check(std::isfinite(drum.ModalEnergy()) && drum.ModalEnergy() > 0,
        "membrane state retains finite stored energy after a restrike");
}

void TestVelocityIsLinear() {
  using namespace tfdsp::percussion;
  auto parameters = DefaultMembraneDrumParameters();
  parameters.routing.enabled = {true, false, false, false, false};
  parameters.observation[0].gain = 1.f;
  parameters.observation[1].gain = 0.f;
  parameters.equalizer.mode = ObservationEqualizerMode::Bypass;
  parameters.outputGain = 1.f;
  const auto hard = Render(48000.f, {1.f, .5f, .5f, 1.f, .25f, 29}, parameters);
  const auto half = Render(48000.f, {.5f, .5f, .5f, 1.f, .25f, 29}, parameters);
  CheckNear(Energy(half) / Energy(hard), .25, 1.e-5,
            "membrane contact amplitude is linear in hit strength");
}

void TestSingleHitEnergyBudget() {
  using namespace tfdsp::percussion;
  const auto parameters = DefaultMembraneDrumParameters();
  MembraneDrum drum;
  drum.Prepare(48000.f, parameters);
  drum.Trigger({1.f, .5f, .8f, 1.f, .2f, 31});
  float maximumEnergy = 0.f;
  for (std::size_t sample = 0; sample < 48000; ++sample) {
    drum.Process();
    maximumEnergy = std::max(maximumEnergy, drum.ModalEnergy());
  }
  Check(maximumEnergy > 0 && maximumEnergy < 4,
        "normalized single-hit energy stays in nominal units without a cap");
}

void TestRatesAndRoutes() {
  using namespace tfdsp::percussion;
  for (const float sampleRate : {44100.f, 48000.f, 96000.f, 192000.f}) {
    auto parameters = DefaultMembraneDrumParameters();
    const auto audio =
        Render(sampleRate, {.8f, .5f, .5f, 1.f, .25f, 9}, parameters);
    Check(std::all_of(audio.begin(), audio.end(),
                      [](const float sample) { return std::isfinite(sample); }),
          "membrane recipe supports every production sample rate");
  }
  auto silentParameters = DefaultMembraneDrumParameters();
  for (std::size_t route = 0; route < silentParameters.routing.enabled.size();
       ++route)
    silentParameters.routing.SetEnabled(route, false);
  const auto silent =
      Render(48000.f, {.8f, .5f, .5f, 1.f, .25f, 9}, silentParameters);
  Check(Energy(silent) < 1.e-20,
        "disabling every membrane route produces silence");
}

} // namespace membrane_test
