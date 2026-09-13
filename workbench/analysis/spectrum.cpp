#include "spectrum.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unsupported/Eigen/FFT>
namespace drumfoundry::analysis {
Spectrogram Analyze(const Audio &audio, const Transform &transform,
                    const std::function<bool()> &cancel) {
  if (audio.samples.empty())
    throw std::invalid_argument("Cannot analyze empty audio");
  SpectrumStream stream(audio.sampleRate, transform);
  return stream.Next(audio.samples, true, cancel);
}
float Spectrogram::At(double seconds, double frequency) const {
  if (!frames || !bins || !std::isfinite(seconds) ||
      !std::isfinite(frequency) || seconds < 0 || frequency < 0 ||
      frequency > sampleRate * .5)
    return -180;
  if (seconds > double(frames) * hop / sampleRate)
    return -180;
  const auto frame = std::size_t(std::llround(seconds * sampleRate / hop));
  const auto bin = std::size_t(std::llround(frequency * size / sampleRate));
  return frame < frames && bin < bins ? db[frame * bins + bin] : -180;
}
} // namespace drumfoundry::analysis
