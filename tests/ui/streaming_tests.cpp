#include "workbench/analysis/live_worker.hpp"
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>
namespace {
void Check(bool value) {
  if (!value)
    throw std::runtime_error("Streaming spectrogram regression");
}
void Collect(drumfoundry::analysis::LiveWorker &worker,
             drumfoundry::analysis::Audio &audio,
             drumfoundry::analysis::Spectrogram &spectrum) {
  for (const auto &chunk : worker.Take()) {
    Check(chunk.firstSample == audio.samples.size());
    Check(chunk.firstFrame == spectrum.frames);
    audio.samples.insert(audio.samples.end(), chunk.audio.samples.begin(),
                         chunk.audio.samples.end());
    drumfoundry::analysis::AppendSpectrum(spectrum, chunk.spectrum);
  }
}
} // namespace
void StreamingTests() {
  using namespace drumfoundry::analysis;
  Audio source{48000, 1, std::vector<float>(12000)};
  for (unsigned i = 0; i < source.samples.size(); ++i)
    source.samples[i] =
        float(.2 * std::sin(.097 * i) + .1 * std::sin(.503 * i));
  for (const auto *window : {"hann", "blackman-harris", "rectangular"}) {
    const Transform transform{1024, 128, window};
    const auto expected = Analyze(source, transform);
    SpectrumStream stream(48000, transform);
    std::vector<float> prefix;
    Spectrogram collected;
    for (unsigned offset = 0; offset < source.samples.size(); offset += 137) {
      const auto count =
          std::min<std::size_t>(137, source.samples.size() - offset);
      prefix.insert(prefix.end(), source.samples.begin() + offset,
                    source.samples.begin() + offset + count);
      const auto chunk =
          stream.Next(prefix, prefix.size() == source.samples.size());
      AppendSpectrum(collected, chunk);
    }
    Check(collected.db == expected.db && collected.frames == expected.frames);
  }
  LiveWorker worker;
  worker.Begin(48000, {1024, 128, "hann"}, .25);
  Audio received;
  Spectrogram spectrum;
  worker.Feed(source.samples.data(), 2400);
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (received.samples.empty() &&
         std::chrono::steady_clock::now() < deadline) {
    Collect(worker, received, spectrum);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  Check(received.samples.size() == 2400 && spectrum.frames > 0 &&
        worker.Active());
  worker.Feed(source.samples.data() + 2400, 9600);
  while (worker.Active() && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  Collect(worker, received, spectrum);
  Check(!worker.Active() && worker.Error().empty());
  Check(received.samples == source.samples);
  Check(spectrum.db == Analyze(source, {1024, 128, "hann"}).db);
  worker.Begin(48000, {512, 128, "hann"}, 1);
  worker.Feed(source.samples.data(), 1000);
  worker.Cancel();
  Check(!worker.Active() && worker.Take().empty());
}
