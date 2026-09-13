#include "worker.hpp"
#include <stdexcept>
namespace drumfoundry::analysis {
void Worker::ReadReference(const Request &request, Result &result,
                           const std::function<bool()> &cancelled) {
  if (request.reference.empty())
    return;
  const auto modified = std::filesystem::last_write_time(request.reference);
  const auto size = std::filesystem::file_size(request.reference);
  const bool reload = cachedPath_ != request.reference ||
                      cachedTime_ != modified || cachedSize_ != size ||
                      cachedChannel_ != request.channel;
  if (reload) {
    cachedPath_.clear(); // A failed decode must not leave a valid cache key.
    auto audio = ReadWave(request.reference, request.channel);
    auto hash = FileHash(request.reference);
    if (cancelled())
      return;
    cachedAudio_ = std::move(audio);
    cachedHash_ = std::move(hash);
    cachedSpectrum_ = {};
    cachedReferencePlayback_.clear();
    cachedPlaybackRate_ = 0;
    cachedPath_ = request.reference;
    cachedTime_ = modified;
    cachedSize_ = size;
    cachedChannel_ = request.channel;
  }
  if (!request.expectedHash.empty() && request.expectedHash != cachedHash_)
    throw std::runtime_error(
        "Reference WAV differs from the saved SHA256 identity");
  const auto &t = request.transform;
  if (!cachedSpectrum_.frames || cachedTransform_.size != t.size ||
      cachedTransform_.hop != t.hop || cachedTransform_.window != t.window) {
    auto spectrum = Analyze(cachedAudio_, t, cancelled);
    if (cancelled())
      return;
    cachedSpectrum_ = std::move(spectrum);
    cachedTransform_ = t;
  }
  result.reference = cachedAudio_;
  result.referenceHash = cachedHash_;
  result.referenceSpectrum = cachedSpectrum_;
}
} // namespace drumfoundry::analysis
