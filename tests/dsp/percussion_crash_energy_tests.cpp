#include "crash_test_support.hpp"

namespace crash_test {

void TestMuteIsPassive() {
  using namespace tfdsp::percussion;
  constexpr float sampleRate = 48000.f;
  CrashCymbal natural;
  CrashCymbal muted;
  const auto parameters = DefaultCrashCymbalParameters(sampleRate);
  natural.Prepare(sampleRate, parameters);
  muted.Prepare(sampleRate, parameters);
  natural.Trigger({.9f, 1.f, .65f, 9});
  muted.Trigger({.9f, 1.f, .65f, 9});
  double naturalTail = 0.0;
  double mutedTail = 0.0;
  for (std::size_t sample = 0; sample < 2 * 48000; ++sample) {
    if (sample == 4800)
      muted.SetMute(1.f);
    const float first = natural.Process();
    const float second = muted.Process();
    if (sample > 12000) {
      naturalTail += static_cast<double>(first) * first;
      mutedTail += static_cast<double>(second) * second;
    }
  }
  Check(mutedTail < .25 * naturalTail,
        "crash mute removes stored energy instead of sustaining it");

  CrashCymbal silent;
  silent.Prepare(sampleRate, parameters);
  silent.SetMute(1.f);
  double silentEnergy = 0.0;
  for (std::size_t sample = 0; sample < 48000; ++sample) {
    const float output = silent.Process();
    silentEnergy += static_cast<double>(output) * output;
  }
  Check(silentEnergy == 0.0, "changing crash mute does not inject energy");

  CrashCymbal openHit;
  CrashCymbal constrainedHit;
  openHit.Prepare(sampleRate, parameters);
  constrainedHit.Prepare(sampleRate, parameters);
  constrainedHit.SetMute(1.f);
  openHit.Trigger({.9f, 1.f, .65f, 19});
  constrainedHit.Trigger({.9f, 1.f, .65f, 19});
  double openHitTail = 0.0;
  double constrainedHitTail = 0.0;
  for (std::size_t sample = 0; sample < 48000; ++sample) {
    const float open = openHit.Process();
    const float constrained = constrainedHit.Process();
    if (sample > 4800) {
      openHitTail += static_cast<double>(open) * open;
      constrainedHitTail += static_cast<double>(constrained) * constrained;
    }
  }
  Check(constrainedHitTail < .25 * openHitTail,
        "a pre-existing constraint damps the hit from its first body response");
}

void TestBloomIsIntrinsicUpwardEnergyTransport() {
  using namespace tfdsp::percussion;
  constexpr float sampleRate = 48000.f;
  CrashCymbalFitParameters fit;
  fit.fieldTurbulence = 0.f;
  fit.fieldExchange = 0.f;
  fit.sparseAmplitude.fill(.001f);
  fit.sparseAmplitude[0] = 1.f;
  fit.bodyDecaySeconds.fill(20.f);
  fit.bloomRateOctavesPerSecond = 8.f;
  fit.bloomEnergyAcceleration = 0.f;
  CrashCymbal cascade;
  cascade.Prepare(sampleRate, DefaultCrashCymbalParameters(sampleRate, fit));
  cascade.Trigger({.8f, .5f, .5f, 71});
  float earlyCentroid = 0.f;
  bool transferredImmediately = false;
  for (int sample = 0; sample < 24000; ++sample) {
    const auto frame = cascade.ProcessFrame();
    if (sample < 64 && frame.bloomTransferEnergy > 0.f)
      transferredImmediately = true;
    if (sample == 1024)
      earlyCentroid = cascade.StoredBodyEnergyCentroidHz();
  }
  const float lateCentroid = cascade.StoredBodyEnergyCentroidHz();
  Check(transferredImmediately,
        "modal bloom begins continuously without a delayed excitation burst");
  Check(lateCentroid > 1.25f * earlyCentroid,
        "stored modal energy travels progressively toward higher frequencies");
}

void TestStrongerStrikeAcceleratesUpwardEnergyTransport() {
  using namespace tfdsp::percussion;
  using Cascade = ModalEnergyCascade<3>;
  constexpr std::array<float, 3> frequencies{500.f, 2000.f, 8000.f};
  constexpr float normalizedGain = .5773502691896258f;
  constexpr std::array<float, 3> gains{normalizedGain, normalizedGain,
                                       normalizedGain};
  constexpr std::array<std::uint16_t, 3> packets{0, 1, 2};
  const std::array<float, 4> strengths{.2f, .45f, .7f, 1.f};
  std::array<float, strengths.size()> centroids{};
  for (std::size_t index = 0; index < strengths.size(); ++index) {
    std::array<float, 3> real{strengths[index], 0.f, 0.f};
    std::array<float, 3> imaginary{};
    Cascade cascade;
    cascade.Prepare(1000.f, frequencies, gains, packets, 3,
                    {8.f, 1.f, 0.f, 71});
    for (int sample = 0; sample < 40; ++sample)
      cascade.Process(real, imaginary);
    double energy = 0.0;
    double weightedLogFrequency = 0.0;
    for (std::size_t mode = 0; mode < real.size(); ++mode) {
      const double modeEnergy =
          static_cast<double>(real[mode]) * real[mode] +
          static_cast<double>(imaginary[mode]) * imaginary[mode];
      energy += modeEnergy;
      weightedLogFrequency += modeEnergy * std::log2(frequencies[mode]);
    }
    centroids[index] =
        static_cast<float>(std::exp2(weightedLogFrequency / energy));
  }
  bool monotonic = true;
  for (std::size_t index = 1; index < centroids.size(); ++index)
    monotonic = monotonic && centroids[index] > centroids[index - 1];
  if (!monotonic) {
    std::cerr << "crash cascade velocity centroids:";
    for (const float centroid : centroids)
      std::cerr << ' ' << centroid;
    std::cerr << '\n';
  }
  Check(monotonic && centroids.back() > 1.1f * centroids.front(),
        "each higher strike energy drives modal energy farther upward");
}

void TestZeroStrengthTriggerIsANoOp() {
  using namespace tfdsp::percussion;
  const auto parameters = DefaultCrashCymbalParameters(48000.f);
  CrashCymbal control;
  CrashCymbal probed;
  control.Prepare(48000.f, parameters);
  probed.Prepare(48000.f, parameters);
  control.Trigger({.9f, .8f, .65f, 93});
  probed.Trigger({.9f, .8f, .65f, 93});
  for (int sample = 0; sample < 4096; ++sample) {
    Check(control.Process() == probed.Process(),
          "equal crash states remain sample-identical");
  }
  probed.Trigger({0.f, 0.f, 0.f, 0xffffffffu, 0.f, 1.f});
  for (int sample = 0; sample < 8192; ++sample)
    Check(control.Process() == probed.Process(),
          "zero-strength crash trigger cannot mutate a ringing body");
}

void TestRestrikeAddsWithoutRecolouringStoredEnergy() {
  using namespace tfdsp::percussion;
  constexpr int Onset = 4096;
  constexpr int Tail = 8192;
  CrashCymbalFitParameters fit;
  fit.bloomRateOctavesPerSecond = 0.f;
  const auto parameters = DefaultCrashCymbalParameters(48000.f, fit);
  CrashCymbal combined;
  CrashCymbal original;
  CrashCymbal added;
  combined.Prepare(48000.f, parameters);
  original.Prepare(48000.f, parameters);
  added.Prepare(48000.f, parameters);
  combined.Trigger({.95f, .1f, .8f, 101});
  original.Trigger({.95f, .1f, .8f, 101});
  for (int sample = 0; sample < Onset; ++sample) {
    combined.Process();
    original.Process();
    added.Process();
  }
  combined.Trigger({.25f, .95f, .3f, 102});
  added.Trigger({.25f, .95f, .3f, 102});
  double error = 0.0;
  double energy = 0.0;
  for (int sample = 0; sample < Tail; ++sample) {
    const double sum = original.Process() + added.Process();
    const double actual = combined.Process();
    const double delta = actual - sum;
    error += delta * delta;
    energy += actual * actual;
  }
  if (!(error < 1.e-7 * std::max(energy, 1.e-30)))
    std::cerr << "crash linear restrike error/energy: " << error << '/'
              << energy << '\n';
  Check(
      error < 1.e-7 * std::max(energy, 1.e-30),
      "a new strike adds to, rather than recolours, stored linear body energy");
}

void TestRepeatedHitsAccumulateBodyEnergy() {
  using namespace tfdsp::percussion;
  constexpr int interval = 6000;
  constexpr int hitCount = 16;
  CrashCymbalFitParameters fit;
  fit.bloomRateOctavesPerSecond = 3.f;
  fit.bloomEnergyAcceleration = .7f;
  CrashCymbal cymbal;
  cymbal.Prepare(48000.f, DefaultCrashCymbalParameters(48000.f, fit));
  std::array<double, hitCount> intervalEnergy{};
  for (int sample = 0; sample < hitCount * interval; ++sample) {
    const int hit = sample / interval;
    if (sample % interval == 0)
      cymbal.Trigger({.45f, 1.f, .65f, static_cast<std::uint32_t>(100 + hit)});
    const auto frame = cymbal.ProcessFrame();
    intervalEnergy[hit] +=
        static_cast<double>(frame.modalBody) * frame.modalBody;
  }
  const double initial = intervalEnergy[0];
  const double earlyPeak =
      *std::max_element(intervalEnergy.begin() + 1, intervalEnergy.begin() + 5);
  const double lateMean =
      .25 * (intervalEnergy[hitCount - 4] + intervalEnergy[hitCount - 3] +
             intervalEnergy[hitCount - 2] + intervalEnergy[hitCount - 1]);
  if (!(earlyPeak > 1.5 * initial && lateMean > .5 * initial)) {
    std::cerr << "crash repeated-hit initial/peak/late energy: " << initial
              << "/" << earlyPeak << "/" << lateMean << " intervals:";
    for (const double energy : intervalEnergy)
      std::cerr << ' ' << energy;
    std::cerr << '\n';
  }
  Check(earlyPeak > 1.5 * initial && lateMean > .5 * initial,
        "repeated crash hits build an initial swell and retain late energy");
}

void TestMaximumBloomRemainsBounded() {
  using namespace tfdsp::percussion;
  CrashCymbalFitParameters fit;
  fit.bloomRateOctavesPerSecond = 16.f;
  fit.bloomEnergyAcceleration = 1.f;
  fit.bloomPhaseDiffusion = 1.f;
  CrashCymbal cymbal;
  cymbal.Prepare(48000.f, DefaultCrashCymbalParameters(48000.f, fit));
  cymbal.Trigger({1.f, 1.f, 1.f, 91});
  for (int sample = 0; sample < 96000; ++sample) {
    const auto frame = cymbal.ProcessFrame();
    Check(std::isfinite(frame.output) && std::abs(frame.output) < 100.f &&
              std::isfinite(cymbal.StoredBodyEnergy()),
          "maximum modal cascade settings remain finite and bounded");
  }
}

} // namespace crash_test
