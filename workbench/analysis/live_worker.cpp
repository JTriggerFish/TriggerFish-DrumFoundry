#include "live_worker.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace drumfoundry::analysis {
LiveWorker::LiveWorker() : thread_([this] { Run(); }) {}
LiveWorker::~LiveWorker() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  wake_.notify_one();
  thread_.join();
}
void LiveWorker::Begin(unsigned rate, Transform transform, double seconds) {
  if (!std::isfinite(seconds) || seconds <= 0 || seconds > 60)
    throw std::invalid_argument("Invalid live capture duration");
  std::lock_guard<std::mutex> lock(mutex_);
  ++revision_;
  rate_ = rate;
  transform_ = std::move(transform);
  total_ = std::size_t(std::ceil(rate * seconds));
  pending_.clear();
  chunks_.clear();
  error_.clear();
  active_ = true;
}
void LiveWorker::Cancel() {
  std::lock_guard<std::mutex> lock(mutex_);
  ++revision_;
  active_ = false;
  pending_.clear();
  chunks_.clear();
}
void LiveWorker::Feed(const float *samples, unsigned count) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!active_ || !count)
    return;
  if (pending_.size() + count > rate_ * 2) {
    active_ = false;
    error_ = "Live analysis fell behind; strike again to restart the display. "
             "Audio is unaffected.";
    return;
  }
  pending_.insert(pending_.end(), samples, samples + count);
  wake_.notify_one();
}
std::vector<PreviewChunk> LiveWorker::Take() {
  std::lock_guard<std::mutex> lock(mutex_);
  return std::exchange(chunks_, {});
}
std::string LiveWorker::Error() {
  std::lock_guard<std::mutex> lock(mutex_);
  return std::exchange(error_, {});
}
void LiveWorker::Run() {
  unsigned current = ~0u;
  std::unique_ptr<SpectrumStream> analyzer;
  std::vector<float> samples;
  for (;;) {
    unsigned revision, rate;
    std::size_t total;
    Transform transform;
    std::vector<float> input;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [this] { return stop_ || !pending_.empty(); });
      if (stop_)
        return;
      revision = revision_;
      rate = rate_;
      transform = transform_;
      total = total_;
      input = std::exchange(pending_, {});
    }
    try {
      if (revision != current) {
        analyzer = std::make_unique<SpectrumStream>(rate, transform);
        samples.clear();
        samples.reserve(total);
        current = revision;
      }
      PreviewChunk chunk;
      chunk.firstSample = samples.size();
      chunk.firstFrame = analyzer->Frames();
      input.resize(std::min(input.size(), total - samples.size()));
      samples.insert(samples.end(), input.begin(), input.end());
      chunk.complete = samples.size() == total;
      chunk.audio = {rate, 1, std::move(input)};
      chunk.spectrum = analyzer->Next(samples, chunk.complete);
      std::lock_guard<std::mutex> lock(mutex_);
      if (revision == revision_) {
        if (chunk.complete) {
          active_ = false;
          pending_.clear();
        }
        chunks_.push_back(std::move(chunk));
      }
    } catch (const std::exception &e) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (revision == revision_) {
        active_ = false;
        pending_.clear();
        error_ = e.what();
      }
    }
  }
}
} // namespace drumfoundry::analysis
