#include "modal_test_support.hpp"

namespace modal_test {

void TestOneModeMatchesAnalyticRecurrence() {
  using Bank = tfdsp::percussion::ModalBank<1>;
  constexpr float sampleRate = 48000.f;
  constexpr float frequency = 1000.f;
  constexpr float decay = 1.f;
  Bank::Parameters parameters{{{frequency, decay, 1.f, 1.f, 0.f}}};
  Bank bank;
  bank.Prepare(sampleRate, parameters, 500.f, 5000.f);
  constexpr double TwoPi = 6.28318530717958647692;
  const double radius = std::exp(std::log(.001) / (decay * sampleRate));
  double maximumError = 0.0;
  double oneSecond = 0.0;
  for (std::size_t sample = 0; sample <= 48000; ++sample) {
    const double actual = bank.Process(sample == 0 ? 1.f : 0.f);
    const double expected =
        std::pow(radius, static_cast<double>(sample)) *
        std::cos(TwoPi * frequency * static_cast<double>(sample) / sampleRate);
    maximumError = std::max(maximumError, std::abs(actual - expected));
    if (sample == 48000)
      oneSecond = actual;
  }
  CheckNear(maximumError, 0.0, 2.e-4,
            "modal recurrence matches its declared analytic response");
  CheckNear(oneSecond, .001, 2.e-6,
            "modal recurrence reaches -60 dB at its declared T60");
}

void TestIndependentProjections() {
  using Bank = tfdsp::percussion::ModalBank<3>;
  Bank::Parameters parameters{{
      {500.f, 2.f, 1.f, 1.f, 0.f},
      {1300.f, 2.f, 1.f, 1.f, 0.f},
      {2700.f, 2.f, 1.f, 1.f, 0.f},
  }};
  Bank bank;
  bank.Prepare(48000.f, parameters, 700.f, 2000.f);
  const Bank::Projection excitation{0.f, .25f, 0.f};
  const Bank::Projection observation{1.f, 2.f, 1.f};
  CheckNear(bank.ProcessProjected(1.f, excitation, observation), .5, 1.e-7,
            "modal excitation and observation projections remain independent");
}

void TestPreparedProjectionIsSanitizedOnce() {
  using Bank = tfdsp::percussion::ModalBank<3>;
  Bank::Parameters parameters{{
      {500.f, 2.f, 1.f, 1.f, 0.f},
      {1300.f, 2.f, 1.f, 1.f, 0.f},
      {2700.f, 2.f, 1.f, 1.f, 0.f},
  }};
  Bank prepared;
  Bank reference;
  prepared.Prepare(48000.f, parameters, 700.f, 2000.f);
  reference.Prepare(48000.f, parameters, 700.f, 2000.f);
  const Bank::Projection unsafe{std::numeric_limits<float>::quiet_NaN(),
                                std::numeric_limits<float>::infinity(), .25f};
  const Bank::Projection sanitized{0.f, 0.f, .25f};
  prepared.SetExcitationProjection(unsafe);
  for (int sample = 0; sample < 128; ++sample) {
    const float input = sample == 0 ? 1.f : 0.f;
    Check(prepared.ProcessExcited(input) ==
              reference.ProcessExcited(input, sanitized),
          "prepared modal projection retains per-sample safety semantics");
  }
}

void TestTwoExcitationsShareOneStoredBody() {
  using Bank = tfdsp::percussion::ModalBank<3>;
  Bank::Parameters parameters{{
      {500.f, 2.f, 1.f, 1.f, 0.f},
      {1300.f, 2.f, 1.f, 1.f, 0.f},
      {2700.f, 2.f, 1.f, 1.f, 0.f},
  }};
  Bank bank;
  bank.Prepare(48000.f, parameters, 700.f, 2000.f);
  bank.SetExcitationProjection({0.f, .25f, 0.f});
  bank.SetSecondaryExcitationProjection({0.f, 0.f, .5f});
  CheckNear(bank.ProcessExcitedPair(1.f, 1.f), .75, 1.e-7,
            "two modal forces add through independent projections");
  Check(std::abs(bank.ProcessExcitedPair(0.f, 0.f)) > 0.f,
        "paired excitation writes one persistent modal state");
}

void TestEnergyNormalizedProjectionOnlyRedistributesDrive() {
  using Field = tfdsp::percussion::StochasticModalField<2>;
  Field::Parameters parameters{{
      {500.f, 2.f, .6f, 1.f, 0.f, 0.f, 0},
      {2000.f, 2.f, .8f, 1.f, 0.f, 0.f, 1},
  }};
  const auto storedAfterStrike = [&](const Field::Projection projection) {
    Field field;
    field.Prepare(48000.f, parameters, {}, 700.f, 1500.f);
    field.SetEnergyNormalizedExcitationProjection(projection);
    field.ProcessExcitedPair(1.f, 0.f);
    return field.StoredEnergy();
  };
  CheckNear(storedAfterStrike({1.f, 0.f}), 1.0, 2.e-7,
            "low-only projection preserves prepared drive energy");
  CheckNear(storedAfterStrike({0.f, .25f}), 1.0, 2.e-7,
            "high-only projection preserves prepared drive energy");
  CheckNear(storedAfterStrike({0.f, 0.f}), 0.0, 0.0,
            "an empty normalized projection remains silent");
}

void TestEnergyNormalizedProjectionSurvivesStaticRebuild() {
  using Field = tfdsp::percussion::StochasticModalField<3>;
  Field::Parameters parameters{{
      {500.f, 2.f, 0.f, 1.f, 0.f, 0.f, 0},
      {1000.f, 2.f, .6f, 1.f, 0.f, 0.f, 1},
      {2000.f, 2.f, .8f, 1.f, 0.f, 0.f, 2},
  }};
  Field field;
  field.Prepare(48000.f, parameters, {}, 700.f, 1500.f);
  field.SetEnergyNormalizedExcitationProjection({0.f, 1.f, 0.f});
  field.SetStaticParameters(parameters);
  field.ProcessExcitedPair(1.f, 0.f);
  CheckNear(field.StoredEnergy(), 1.0, 2.e-7,
            "normalized source-index projection survives a static rebuild");
}

void TestModalConstraintReferenceGain() {
  using namespace tfdsp::percussion;
  ModalConstraintController controller;
  controller.Prepare(48000.f, .001f, 0.f, 0.f);
  controller.SetTarget({.5f, .25f, .125f, .0625f});
  double broadband = 1.0;
  double low = 1.0;
  for (int sample = 0; sample < 48; ++sample) {
    const auto gain = controller.Process();
    broadband *= gain.broadband;
    low *= gain.low;
  }
  CheckNear(broadband, .5, 2.e-6,
            "modal constraint realizes broadband traversal attenuation");
  CheckNear(low, .25, 2.e-6,
            "modal constraint realizes band traversal attenuation");
}

} // namespace modal_test
