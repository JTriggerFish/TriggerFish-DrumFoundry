#include "adapters/shared/audio_tap.hpp"
#include "workbench/analysis/live_spectrum.hpp"
#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
namespace {
void Check(bool value) {
  if (!value)
    throw std::runtime_error("Live spectrum/tap regression");
}
void TapTests() {
  using drumfoundry::host::AudioTap;
  AudioTap tap;
  tap.Reset(48000);
  std::vector<float> samples(AudioTap::Capacity, .25f), read(samples.size());
  tap.Push(samples.data(), unsigned(samples.size()));
  auto info = tap.Read(read.data(), unsigned(read.size()));
  Check(info.samples == samples.size() && info.rate == 48000 &&
        read == samples);
  tap.Push(samples.data(), unsigned(samples.size()));
  tap.Push(samples.data(), 1);
  info = tap.Read(read.data(), unsigned(read.size()));
  Check(info.samples == 0 && info.dropped == 1); // Stale backlog is discarded.
  tap.Push(samples.data(), 13);
  Check(tap.Read(read.data(), 32).samples == 13);
  tap.Reset(44100);
  Check(tap.Read(read.data(), 32).rate == 44100);
  tap.Push(samples.data(), 5);
  tap.Push(samples.data(), 7, true);
  tap.Push(samples.data(), 3, true);
  info = tap.Read(read.data(), 32);
  Check(info.samples == 15 && info.firstStrike == 5 && info.lastStrike == 12);
  std::atomic<bool> done{};
  std::thread producer([&] {
    std::array<float, 128> block{};
    for (unsigned i = 0; i < 10000; ++i) {
      for (unsigned j = 0; j < block.size(); ++j)
        block[j] = float(i * 128 + j + 1);
      tap.Push(block.data(), unsigned(block.size()));
      if (i % 8 == 0)
        std::this_thread::yield();
    }
    done = true;
  });
  float previous = 0;
  bool ordered = true;
  do {
    info = tap.Read(read.data(), unsigned(read.size()));
    for (unsigned i = 0; i < info.samples; ++i) {
      ordered = ordered && read[i] > previous;
      previous = read[i];
    }
  } while (!done || info.samples);
  producer.join();
  Check(
      ordered); // Wraparound/overflow never exposes partially written samples.
}
} // namespace
void LiveSpectrumTests() {
  using drumfoundry::analysis::LiveSpectrum;
  TapTests();
  LiveSpectrum meter, chunked;
  meter.Reset(32768);
  chunked.Reset(32768);
  std::vector<float> tone(LiveSpectrum::Size);
  for (unsigned i = 0; i < tone.size(); ++i)
    tone[i] = float(.25 * std::sin(6.283185307179586 * 100 * i / tone.size()));
  meter.Update(tone.data(), unsigned(tone.size()));
  Check(std::abs(meter.Decibels()[100] - 20 * std::log10(.25)) < .001);
  for (unsigned i = 0; i < tone.size(); i += 128)
    chunked.Update(tone.data() + i, 128);
  Check(meter.Decibels() == chunked.Decibels());
  meter.Reset(48000);
  std::fill(tone.begin(), tone.end(), .25f);
  meter.Update(tone.data(), unsigned(tone.size()));
  Check(std::abs(meter.Decibels()[0] - 20 * std::log10(.25)) < .001);
  meter.Reset(0);
  Check(meter.Rate() == 0 && meter.Decibels()[0] == -120);
  meter.Reset(48000);
  tone[0] = std::numeric_limits<float>::quiet_NaN();
  bool rejected = false;
  try {
    meter.Update(tone.data(), unsigned(tone.size()));
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  Check(rejected && meter.Decibels()[0] == -120);
}
