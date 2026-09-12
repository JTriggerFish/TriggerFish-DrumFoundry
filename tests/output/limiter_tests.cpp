#include "output/limiter.hpp"
#include "percussion_test_support.hpp"
#include "runtime/voice.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

using drumfoundry::output::Limiter;
using percussion_test::Check;
using percussion_test::CheckNear;
namespace {
constexpr double Pi = 3.14159265358979323846;
void TestInvalidPreparation() {
  drumfoundry::output::PeakDetector detector;
  for (std::size_t radius : {0u, 33u}) {
    try {
      detector.Prepare(radius);
      Check(false, "invalid detector radius rejected");
    } catch (const std::invalid_argument &) {
    }
  }
  drumfoundry::output::GainAverage average;
  try {
    average.Prepare(0);
    Check(false, "zero smoother length rejected");
  } catch (const std::invalid_argument &) {
  }
  Limiter limiter;
  limiter.Prepare(48000);
  try {
    limiter.Prepare(0);
    Check(false, "invalid rate rejected");
  } catch (const std::invalid_argument &) {
  }
  Check(limiter.Status().latencySamples == 48,
        "invalid preparation preserves valid configuration");
  Check(!limiter.Process(nullptr, 1), "missing audio buffers rejected");
}
double Peak(const std::vector<float> &x) {
  double peak = 0;
  for (auto value : x)
    peak = std::max(peak, std::abs(double(value)));
  return peak;
}
std::vector<float> Render(Limiter &limiter, std::vector<float> input,
                          const std::vector<std::size_t> &blocks = {128}) {
  std::size_t cursor = 0, block = 0;
  while (cursor < input.size()) {
    auto *pointer = input.data() + cursor;
    const auto frames =
        std::min(blocks[block++ % blocks.size()], input.size() - cursor);
    Check(limiter.Process(&pointer, frames), "valid mono block");
    cursor += frames;
  }
  return input;
}
void TestLatencyAndBypass() {
  for (const double rate : {8000., 44100., 48000., 96000., 192000., 384000.}) {
    for (bool enabled : {false, true}) {
      Limiter limiter;
      limiter.Prepare(rate, 1, enabled);
      const auto delay = enabled ? std::lround(rate * .001) : 0;
      Check(limiter.Status().latencySamples == static_cast<std::size_t>(delay),
            "reported latency");
      std::vector<float> input(1024);
      input[3] = .01f;
      auto output = Render(limiter, input);
      for (std::size_t i = 0; i < output.size(); ++i)
        Check(output[i] == (i == 3 + delay ? .01f : 0.f),
              "quiet impulse has exact latency, no extra delay");
    }
  }
  Limiter bypass;
  bypass.Prepare(48000, 1, false);
  const std::vector<float> input{2.f, -4.f, 0.f, .15f};
  Check(Render(bypass, input) == input,
        "bypass does not clip or normalize finite audio");
}
void TestBoundariesAndOverloads() {
  std::vector<float> input(12000);
  for (std::size_t i = 0; i < input.size(); ++i)
    input[i] = static_cast<float>(2. * std::sin(.87 * i) *
                                  std::exp(-double(i) / 4000));
  for (auto i : {0, 1, 127, 128, 255, 256, 511, 512, 4095})
    input[i] = i % 2 ? -20.f : 20.f;
  Limiter first, second;
  first.Prepare(48000);
  second.Prepare(48000);
  const auto a = Render(first, input, {1});
  const auto b = Render(second, input, {128, 512, 7, 1, 256});
  Check(a == b, "buffer partitioning does not alter limiter output");
  Check(Peak(a) <= std::pow(10., Limiter::CeilingDb / 20) + 1e-6,
        "all boundary peaks contained");
  Check(first.Status().maximumReductionDb > 20, "limiting is visible in meter");
  Check(first.Status().inputPeakDb > 20, "pre-limiter overload visible");
  const auto current = first.Status().reductionDb;
  first.ClearMeters();
  CheckNear(first.Status().reductionDb, current, 1e-12,
            "meter reset preserves audio gain");
  first.Reset();
  input.assign(1024, 0);
  input[0] = std::numeric_limits<float>::max();
  input[1] = std::numeric_limits<float>::quiet_NaN();
  input[2] = std::numeric_limits<float>::infinity();
  const auto extreme = Render(first, input);
  Check(first.Status().invalidInput, "nonfinite input is reported");
  Check(std::all_of(extreme.begin(), extreme.end(),
                    [](float x) { return std::isfinite(x); }),
        "extreme input remains finite");
  Check(Peak(extreme) < .9, "extreme input does not defeat gain smoothing");
}
void TestBassAndStereo() {
  Limiter limiter;
  limiter.Prepare(48000, 2);
  double correlation = 0, energy = 0, residual = 0;
  std::vector<double> output;
  for (std::size_t i = 0; i < 96000; ++i) {
    const float sample =
        static_cast<float>(4 * std::sin(2 * Pi * 20 * i / 48000));
    const auto frame = limiter.ProcessFrame({sample, -.25f * sample});
    CheckNear(frame[1], -.25 * frame[0], 1e-7, "stereo gain remains linked");
    if (i >= 48000) {
      const double reference = std::sin(2 * Pi * 20 * (double(i) - 48) / 48000);
      correlation += frame[0] * reference;
      energy += reference * reference;
      output.push_back(frame[0]);
    }
  }
  const double amplitude = correlation / energy;
  for (std::size_t i = 0; i < output.size(); ++i) {
    const double expected =
        amplitude * std::sin(2 * Pi * 20 * (double(i) + 48000 - 48) / 48000);
    residual += std::pow(output[i] - expected, 2);
  }
  Check(std::sqrt(residual / energy) / amplitude < .002,
        "20 Hz sustained limiting has little gain flutter");
}
// Independent 16x/193-tap reconstruction, not the production 4x detector.
double ReconstructedPeak(const std::vector<float> &audio, std::size_t start,
                         std::size_t length) {
  double peak = 0;
  constexpr int radius = 96;
  for (int phase = 0; phase < 16; ++phase) {
    std::array<double, 2 * radius + 1> taps{};
    double total = 0;
    for (int tap = -radius; tap <= radius; ++tap) {
      const double x = tap - phase / 16.;
      const double sinc = x == 0 ? 1 : std::sin(Pi * x) / (Pi * x);
      const double window = std::abs(x) < radius
                                ? .42 + .5 * std::cos(Pi * x / radius) +
                                      .08 * std::cos(2 * Pi * x / radius)
                                : 0;
      taps[tap + radius] = sinc * window;
      total += taps[tap + radius];
    }
    for (std::size_t i = start; i < start + length; ++i) {
      double sample = 0;
      for (int tap = -radius; tap <= radius; ++tap)
        sample += audio[i + tap] * taps[tap + radius] / total;
      peak = std::max(peak, std::abs(sample));
    }
  }
  return peak;
}
void TestHighFrequencies() {
  for (double frequency : {.12, .249, .45, .49}) {
    Limiter limiter;
    limiter.Prepare(48000);
    std::vector<float> input(12000);
    for (std::size_t i = 0; i < input.size(); ++i)
      input[i] = static_cast<float>(4 * std::sin(2 * Pi * frequency * i + .43));
    const auto output = Render(limiter, input);
    const double peak = ReconstructedPeak(output, 7000, 512);
    Check(peak <= std::pow(10., Limiter::CeilingDb / 20) + .001,
          "16x reconstructed high-frequency peak contained");
    limiter.Reset();
    input.assign(2048, 0);
    for (std::size_t i = 512; i < 768; ++i)
      input[i] = static_cast<float>(4 * std::sin(2 * Pi * frequency * i + .43));
    const auto burst = Render(limiter, input, {7, 128, 1});
    Check(ReconstructedPeak(burst, 400, 512) <=
              std::pow(10., Limiter::CeilingDb / 20) + .001,
          "16x reconstructed abrupt burst peak contained");
  }
}
void TestRepeatedVoices() {
  for (const auto *recipe : {"drum.kick.v1", "metal.cymbal.v1"}) {
    drumfoundry::Voice voice(48000, drumfoundry::DefaultPatch(recipe));
    Limiter limiter;
    limiter.Prepare(48000);
    for (std::size_t i = 0; i < 96000; ++i) {
      if (i % 6000 == 0)
        voice.Trigger({1, .5f, 1, 1, .2f, 0, 17});
      float sample;
      voice.Process(&sample, 1);
      const auto output = limiter.ProcessFrame({sample * 32, 0});
      Check(std::isfinite(output[0]) && std::abs(output[0]) < .9f,
            "repeated hot drum hits protected");
    }
    Check(limiter.Status().maximumReductionDb > 0,
          "repeated-hit gain reduction visible");
  }
}
} // namespace
int main() {
  TestInvalidPreparation();
  TestLatencyAndBypass();
  TestBoundariesAndOverloads();
  TestBassAndStereo();
  TestHighFrequencies();
  TestRepeatedVoices();
  return percussion_test::failures ? 1 : 0;
}
