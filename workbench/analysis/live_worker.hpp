#pragma once
#include "progressive.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
namespace drumfoundry::analysis {
// GUI submits copied monitor blocks; all FFT work happens on this worker.
// Repeated strikes keep their real overlap; there is no second synthesizer.
class LiveWorker {
public:
  LiveWorker();
  ~LiveWorker();
  void Begin(unsigned rate, Transform transform, double seconds);
  void Feed(const float *samples, unsigned count);
  void Cancel();
  bool Active() const { return active_; }
  std::vector<PreviewChunk> Take();
  std::string Error();

private:
  void Run();
  std::mutex mutex_;
  std::condition_variable wake_;
  unsigned revision_{}, rate_{};
  Transform transform_;
  std::size_t total_{};
  std::vector<float> pending_;
  std::vector<PreviewChunk> chunks_;
  std::string error_;
  std::atomic<bool> active_{};
  bool stop_{};
  std::thread thread_;
};
} // namespace drumfoundry::analysis
