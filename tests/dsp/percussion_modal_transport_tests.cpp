#include "modal_test_support.hpp"

namespace modal_test {

void TestModalCascadeIsPassiveAndDoesNotRetransferArrivals() {
  using Cascade = tfdsp::percussion::ModalEnergyCascade<3>;
  const std::array<float, 3> frequencies{500.f, 2000.f, 8000.f};
  const std::array<float, 3> gains{1.f, 1.f, 1.f};
  const std::array<std::uint16_t, 3> packets{0, 1, 2};
  std::array<float, 3> real{1.f, 0.f, 0.f};
  std::array<float, 3> imaginary{};
  Cascade cascade;
  cascade.Prepare(100.f, frequencies, gains, packets, 3, {12.f, 0.f, 1.f, 91});
  const auto energy = [&] {
    double result = 0.0;
    for (std::size_t index = 0; index < real.size(); ++index)
      result += static_cast<double>(real[index]) * real[index] +
                static_cast<double>(imaginary[index]) * imaginary[index];
    return result;
  };
  const double initialEnergy = energy();
  cascade.Process(real, imaginary);
  CheckNear(energy(), initialEnergy, 2.e-7,
            "modal cascade redistributes rather than creates energy");
  Check(real[1] != 0.f && real[2] == 0.f && imaginary[2] == 0.f,
        "modal cascade does not retransfer new arrivals in one sample");
  cascade.Process(real, imaginary);
  Check(real[2] != 0.f || imaginary[2] != 0.f,
        "modal cascade reaches successive bands progressively");
  CheckNear(energy(), initialEnergy, 3.e-7,
            "successive modal transfers remain energy preserving");
}

void TestModalCascadeIsIndependentOfPaintedAnchorDensity() {
  using Cascade = tfdsp::percussion::ModalEnergyCascade<17>;
  const auto highestEnergy = [](const std::size_t count) {
    std::array<float, 17> frequencies{};
    std::array<float, 17> gains{};
    std::array<std::uint16_t, 17> packets{};
    std::array<float, 17> real{};
    std::array<float, 17> imaginary{};
    for (std::size_t index = 0; index < count; ++index) {
      frequencies[index] = 500.f * std::exp2(4.f * static_cast<float>(index) /
                                             static_cast<float>(count - 1));
      gains[index] = 1.f / std::sqrt(static_cast<float>(count));
      packets[index] = static_cast<std::uint16_t>(index);
    }
    real[0] = 1.f;
    Cascade cascade;
    cascade.Prepare(1000.f, frequencies, gains, packets, count,
                    {4.f, 1.f, 0.f, 83});
    for (int sample = 0; sample < 600; ++sample)
      cascade.Process(real, imaginary);
    return real[count - 1] * real[count - 1] +
           imaginary[count - 1] * imaginary[count - 1];
  };
  CheckNear(highestEnergy(9), highestEnergy(17), 2.e-5,
            "intermediate painted anchors do not slow upward travel");
}

void TestModalCascadePreservesPacketTurbulenceWeights() {
  using Cascade = tfdsp::percussion::ModalEnergyCascade<3>;
  const std::array<float, 3> frequencies{500.f, 1000.f, 1100.f};
  const std::array<float, 3> gains{1.f, std::sqrt(.75f), std::sqrt(.25f)};
  const std::array<std::uint16_t, 3> packets{0, 1, 1};
  std::array<float, 3> real{1.f, 0.f, 0.f};
  std::array<float, 3> imaginary{};
  Cascade cascade;
  cascade.Prepare(1000.f, frequencies, gains, packets, 3, {4.f, 0.f, 0.f, 89});
  cascade.Process(real, imaginary);
  const float coreEnergy = real[1] * real[1] + imaginary[1] * imaginary[1];
  const float satelliteEnergy = real[2] * real[2] + imaginary[2] * imaginary[2];
  CheckNear(coreEnergy / satelliteEnergy, 3.0, 2.e-5,
            "cascade arrivals retain the painted packet's centre-to-turbulence "
            "balance");
}

void TestModalCascadeRateIncludesVerySlowTravel() {
  using Cascade = tfdsp::percussion::ModalEnergyCascade<2>;
  constexpr std::array<float, 2> frequencies{500.f, 1000.f};
  constexpr std::array<float, 2> gains{1.f, 1.f};
  constexpr std::array<std::uint16_t, 2> packets{0, 1};
  const auto upperEnergy = [&](const float rate) {
    std::array<float, 2> real{1.f, 0.f};
    std::array<float, 2> imaginary{};
    Cascade cascade;
    cascade.Prepare(1000.f, frequencies, gains, packets, 2,
                    {rate, 0.f, 0.f, 71});
    for (int sample = 0; sample < 100; ++sample)
      cascade.Process(real, imaginary);
    return real[1] * real[1] + imaginary[1] * imaginary[1];
  };
  const float stopped = upperEnergy(0.f);
  const float verySlow = upperEnergy(.1f);
  const float medium = upperEnergy(1.f);
  const float fast = upperEnergy(4.f);
  Check(stopped == 0.f && verySlow > 0.f && verySlow < .02f &&
            verySlow < medium && medium < fast,
        "modal cascade rate spans off, very slow, medium, and fast travel");
}

void TestModalCascadeEnergyAccelerationOnlyAccelerates() {
  using Cascade = tfdsp::percussion::ModalEnergyCascade<2>;
  constexpr std::array<float, 2> frequencies{500.f, 1000.f};
  constexpr float normalizedGain = .7071067811865475f;
  constexpr std::array<float, 2> gains{normalizedGain, normalizedGain};
  constexpr std::array<std::uint16_t, 2> packets{0, 1};
  const auto upperEnergy = [&](const float strength,
                               const float energyAcceleration) {
    std::array<float, 2> real{strength, 0.f};
    std::array<float, 2> imaginary{};
    Cascade cascade;
    cascade.Prepare(1000.f, frequencies, gains, packets, 2,
                    {2.f, energyAcceleration, 0.f, 97});
    for (int sample = 0; sample < 100; ++sample)
      cascade.Process(real, imaginary);
    return real[1] * real[1] + imaginary[1] * imaginary[1];
  };
  const float baseline = upperEnergy(.001f, 0.f);
  const float quietDependent = upperEnergy(.001f, 1.f);
  const float loudDependent = upperEnergy(1.f, 1.f);
  Check(quietDependent >= .999f * baseline,
        "energy acceleration never reduces the declared baseline rate");
  Check(loudDependent > 1.25f * upperEnergy(1.f, 0.f),
        "stored strike energy accelerates the field-wide cascade");
}

} // namespace modal_test
