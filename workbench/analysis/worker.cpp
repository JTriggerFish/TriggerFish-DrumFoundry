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
void Worker::Cancel() {
  std::lock_guard<std::mutex> lock(mutex_);
  ++revision_;
  pending_.reset();
  result_.reset();
  context_.reset();
  chunks_.clear();
  busy_ = false;
}
void Worker::Submit(Request request) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_ = std::move(request);
    ++revision_;
    result_.reset();
    context_.reset();
    chunks_.clear();
    busy_ = true;
    progress_ = 0;
  }
  wake_.notify_one();
}
std::shared_ptr<const Result> Worker::Take() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (result_) {
    context_.reset();
    chunks_.clear();
  }
  return std::exchange(result_, {});
}
std::shared_ptr<const Result> Worker::TakeContext() {
  std::lock_guard<std::mutex> lock(mutex_);
  return std::exchange(context_, {});
}
std::vector<PreviewChunk> Worker::TakeChunks() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (context_)
    return {}; // Publish context before its first delta, atomically.
  return std::exchange(chunks_, {});
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
  result.auditionRate = request.auditionRate;
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
  result.model = {rate, 1, {}};
  const auto total = std::size_t(std::ceil(duration * rate));
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (cancelled())
      return {};
    context_ = std::make_shared<Result>(result);
  }
  result.model.samples.reserve(total);
  SpectrumStream analyzer(rate, request.transform);
  Voice voice(float(rate), request.document);
  voice.Trigger(voice.Event());
  std::size_t published = 0;
  auto lastPublish = std::chrono::steady_clock::now();
  for (std::size_t offset = 0; offset < total; offset += 2048) {
    if (cancelled())
      return {};
    const auto count = std::min<std::size_t>(2048, total - offset);
    result.model.samples.resize(offset + count);
    voice.Process(result.model.samples.data() + offset, count);
    const auto now = std::chrono::steady_clock::now();
    if (published == 0 || offset + count == total ||
        now - lastPublish >= std::chrono::milliseconds(50)) {
      PreviewChunk chunk;
      chunk.firstSample = published;
      chunk.firstFrame = analyzer.Frames();
      chunk.complete = offset + count == total;
      chunk.audio = {rate,
                     1,
                     {result.model.samples.begin() + published,
                      result.model.samples.end()}};
      chunk.spectrum = analyzer.Next(result.model.samples, chunk.complete);
      AppendSpectrum(result.modelSpectrum, chunk.spectrum);
      published = result.model.samples.size();
      lastPublish = now;
      std::lock_guard<std::mutex> lock(mutex_);
      if (cancelled())
        return {};
      chunks_.push_back(std::move(chunk));
    }
    progress_ = .9f * float(offset + count) / total;
  }
  progress_ = .9f;
  if (cancelled())
    return {};
  result.auditionRate = request.auditionRate;
  result.modelPlayback = Resample(result.model, request.auditionRate);
  if (cancelled())
    return {};
  result.elapsedMs = std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start)
                         .count();
  return result;
}
} // namespace drumfoundry::analysis
