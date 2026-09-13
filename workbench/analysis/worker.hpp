#pragma once
#include "progressive.hpp"
#include "runtime/voice.hpp"
#include "spectrum.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
namespace drumfoundry::analysis {
struct Request {
  Json document;
  Transform transform;
  std::filesystem::path reference;
  std::string expectedHash;
  Channel channel{Channel::MonoAverage};
  double duration{8};
  unsigned auditionRate{48000};
};
struct Result {
  Json document;
  Audio model, reference;
  Spectrogram modelSpectrum, referenceSpectrum;
  double elapsedMs{};
  std::string error;
  std::string referenceHash;
  unsigned auditionRate{};
  std::vector<float> modelPlayback, referencePlayback;
};
// Latest-request-wins worker; owns an independent Voice, never the live voice.
// Cancellation is checked at render blocks and FFT frames. Results are
// immutable.
class Worker {
public:
  Worker();
  ~Worker();
  void Submit(Request);
  void Cancel();
  std::shared_ptr<const Result> Take();
  std::shared_ptr<const Result> TakeContext();
  std::vector<PreviewChunk> TakeChunks();
  float Progress() const { return progress_.load(); }
  bool Busy() const { return busy_.load(); }

private:
  void ReadReference(const Request &, Result &, const std::function<bool()> &);
  // Worker-thread-only cache. File identity is checked before every reuse.
  std::filesystem::path cachedPath_;
  std::filesystem::file_time_type cachedTime_{};
  std::uintmax_t cachedSize_{};
  Channel cachedChannel_{};
  Transform cachedTransform_;
  Audio cachedAudio_;
  Spectrogram cachedSpectrum_;
  std::string cachedHash_;
  std::vector<float> cachedReferencePlayback_;
  unsigned cachedPlaybackRate_{};
  void Run();
  Result Execute(const Request &, unsigned revision);
  std::mutex mutex_;
  std::condition_variable wake_;
  std::optional<Request> pending_;
  std::shared_ptr<const Result> result_;
  std::shared_ptr<const Result> context_;
  std::vector<PreviewChunk> chunks_;
  std::atomic<unsigned> revision_{};
  std::atomic<bool> stop_{}, busy_{};
  std::atomic<float> progress_{};
  std::thread thread_;
};
} // namespace drumfoundry::analysis
