#include "worker.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
namespace drumfoundry::analysis {
Worker::Worker() : thread_([this] { Run(); }) {}
Worker::~Worker() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
    ++revision_;
  }
  wake_.notify_one();
  thread_.join();
}
void Worker::Submit(Request request) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_ = std::move(request);
    ++revision_;
    result_.reset();
    busy_ = true;
    progress_ = 0;
  }
  wake_.notify_one();
}
std::shared_ptr<const Result> Worker::Take() {
  std::lock_guard<std::mutex> lock(mutex_);
  return std::exchange(result_, {});
}
void Worker::Run() {
  for (;;) {
    Request request;
    unsigned revision;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wake_.wait(lock, [this] { return stop_ || pending_.has_value(); });
      if (stop_)
        return;
      request = std::move(*pending_);
      pending_.reset();
      revision = revision_;
    }
    auto result = std::make_shared<Result>();
    try {
      *result = Execute(request, revision);
    } catch (const std::exception &e) {
      result->error = e.what();
    }
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (revision == revision_) {
        result_ = std::move(result);
        busy_ = false;
        progress_ = 1;
      }
    }
  }
}
Result Worker::Execute(const Request &request, unsigned revision) {
  const auto start = std::chrono::steady_clock::now();
  const auto cancelled = [this, revision] {
    return stop_ || revision != revision_;
  };
  Result result;
  result.document = request.document;
  ReadReference(request, result, cancelled);
  if (cancelled())
    return {};
  const unsigned rate =
      result.reference.sampleRate ? result.reference.sampleRate : 48000;
  double duration = request.duration;
  if (duration == 0 && !result.reference.samples.empty())
    duration = double(result.reference.samples.size()) / rate;
  if (!std::isfinite(duration) || duration <= 0 || duration > 60)
    throw std::invalid_argument(
        "Render duration must be above zero and at most 60 seconds");
  result.model = {rate, 1,
                  std::vector<float>(std::size_t(std::ceil(duration * rate)))};
  Voice voice(float(rate), request.document);
  voice.Trigger(voice.Event());
  for (std::size_t offset = 0; offset < result.model.samples.size();
       offset += 2048) {
    if (cancelled())
      return {};
    voice.Process(
        result.model.samples.data() + offset,
        std::min<std::size_t>(2048, result.model.samples.size() - offset));
    progress_ = .75f * float(offset) / result.model.samples.size();
  }
  result.modelSpectrum = Analyze(result.model, request.transform, cancelled);
  progress_ = .9f;
  if (cancelled())
    return {};
  result.auditionRate = request.auditionRate;
  result.modelPlayback = Resample(result.model, request.auditionRate);
  if (cancelled())
    return {};
  if (!result.reference.samples.empty()) {
    if (cachedPlaybackRate_ != request.auditionRate) {
      auto playback = Resample(result.reference, request.auditionRate);
      if (cancelled())
        return {};
      cachedReferencePlayback_ = std::move(playback);
      cachedPlaybackRate_ = request.auditionRate;
    }
    result.referencePlayback = cachedReferencePlayback_;
  }
  result.elapsedMs = std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start)
                         .count();
  return result;
}
} // namespace drumfoundry::analysis
