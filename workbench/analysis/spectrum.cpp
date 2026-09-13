#include "spectrum.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unsupported/Eigen/FFT>
namespace drumfoundry::analysis {
namespace {
std::vector<float> Window(const Transform &t) {
  std::vector<float> result(t.size);
  constexpr double Pi = 3.14159265358979323846;
  for (unsigned i = 0; i < t.size; ++i) {
    const double a = 2 * Pi * i / t.size;
    result[i] = t.window == "hann" ? float(.5 - .5 * std::cos(a))
                : t.window == "blackman-harris"
                    ? float(.35875 - .48829 * std::cos(a) +
                            .14128 * std::cos(2 * a) - .01168 * std::cos(3 * a))
                    : 1;
  }
  return result;
}
} // namespace
Spectrogram Analyze(const Audio &audio, const Transform &t,
                    const std::function<bool()> &cancel) {
  if (t.size < 64 || t.size > 32768 || (t.size & (t.size - 1)) || !t.hop ||
      t.hop > t.size ||
      (t.window != "hann" && t.window != "blackman-harris" &&
       t.window != "rectangular") ||
      audio.sampleRate < 8000 || audio.sampleRate > 384000 ||
      audio.samples.empty())
    throw std::invalid_argument("Invalid STFT settings or empty audio");
  const auto frames = unsigned((audio.samples.size() + t.hop - 1) / t.hop);
  for (float sample : audio.samples)
    if (!std::isfinite(sample))
      throw std::invalid_argument("Audio contains nonfinite samples");
  const unsigned bins = t.size / 2 + 1;
  if (uint64_t(frames) * bins > 64 * 1024 * 1024)
    throw std::invalid_argument(
        "STFT exceeds 256 MB; reduce overlap or render duration");
  Spectrogram result{
      audio.sampleRate, t.size, t.hop,
      frames,           bins,   std::vector<float>(std::size_t(frames) * bins)};
  auto window = Window(t);
  double sum = 0;
  for (auto w : window)
    sum += w;
  Eigen::FFT<float> fft;
  std::vector<float> input(t.size);
  std::vector<std::complex<float>> output;
  for (unsigned frame = 0; frame < frames; ++frame) {
    if (cancel && cancel())
      return {};
    const auto start = int64_t(frame) * t.hop - t.size / 2;
    for (unsigned i = 0; i < t.size; ++i) {
      const auto index = start + i;
      input[i] = index >= 0 && uint64_t(index) < audio.samples.size()
                     ? audio.samples[std::size_t(index)] * window[i]
                     : 0;
    }
    fft.fwd(output, input);
    for (unsigned bin = 0; bin < bins; ++bin) {
      const double gain = bin == 0 || bin == t.size / 2 ? 1 / sum : 2 / sum;
      const float db =
          float(10 * std::log10(std::max(1e-18, double(std::norm(output[bin])) *
                                                    gain * gain)));
      result.db[std::size_t(frame) * bins + bin] = db;
      result.maximumDb = std::max(result.maximumDb, db);
    }
  }
  return result;
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
