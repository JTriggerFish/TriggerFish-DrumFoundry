#include "spectrum.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unsupported/Eigen/FFT>
namespace drumfoundry::analysis {
struct SpectrumStream::Impl {
  unsigned rate, frames{};
  std::size_t validated{};
  Transform transform;
  std::vector<float> window, input;
  std::vector<std::complex<float>> output;
  Eigen::FFT<float> fft;
  double sum{};
  Impl(unsigned r, Transform t) : rate(r), transform(std::move(t)) {
    if (r < 8000 || r > 384000 || transform.size < 64 ||
        transform.size > 32768 || (transform.size & (transform.size - 1)) ||
        !transform.hop || transform.hop > transform.size ||
        (transform.window != "hann" && transform.window != "blackman-harris" &&
         transform.window != "rectangular"))
      throw std::invalid_argument("Invalid STFT settings");
    window.resize(transform.size);
    input.resize(transform.size);
    for (unsigned i = 0; i < transform.size; ++i) {
      const double a = 6.28318530717958647692 * i / transform.size;
      window[i] =
          transform.window == "hann" ? float(.5 - .5 * std::cos(a))
          : transform.window == "blackman-harris"
              ? float(.35875 - .48829 * std::cos(a) + .14128 * std::cos(2 * a) -
                      .01168 * std::cos(3 * a))
              : 1.f;
      sum += window[i];
    }
  }
};
SpectrumStream::SpectrumStream(unsigned rate, Transform t)
    : impl_(std::make_unique<Impl>(rate, std::move(t))) {}
SpectrumStream::~SpectrumStream() = default;
unsigned SpectrumStream::Frames() const { return impl_->frames; }
Spectrogram SpectrumStream::Next(const std::vector<float> &samples, bool finish,
                                 const std::function<bool()> &cancel) {
  auto &s = *impl_;
  const auto &t = s.transform;
  if (samples.size() < s.validated)
    throw std::invalid_argument("Streaming audio cannot move backwards");
  for (std::size_t i = s.validated; i < samples.size(); ++i)
    if (!std::isfinite(samples[i]))
      throw std::invalid_argument("Audio contains nonfinite samples");
  s.validated = samples.size();
  const unsigned end =
      finish ? unsigned((samples.size() + t.hop - 1) / t.hop)
      : samples.size() < t.size / 2
          ? 0
          : unsigned((samples.size() - t.size / 2) / t.hop + 1);
  if (end < s.frames)
    throw std::invalid_argument("Streaming STFT cannot move backwards");
  const unsigned bins = t.size / 2 + 1;
  if (uint64_t(end) * bins > 64 * 1024 * 1024)
    throw std::invalid_argument(
        "STFT exceeds 256 MB; reduce overlap or render duration");
  Spectrogram result{
      s.rate, t.size,
      t.hop,  end - s.frames,
      bins,   std::vector<float>(std::size_t(end - s.frames) * bins)};
  for (unsigned frame = s.frames; frame < end; ++frame) {
    if (cancel && cancel())
      return {};
    const auto start = int64_t(frame) * t.hop - t.size / 2;
    for (unsigned i = 0; i < t.size; ++i) {
      const auto index = start + i;
      s.input[i] = index >= 0 && uint64_t(index) < samples.size()
                       ? samples[std::size_t(index)] * s.window[i]
                       : 0;
    }
    s.fft.fwd(s.output, s.input);
    for (unsigned bin = 0; bin < bins; ++bin) {
      const double gain = bin == 0 || bin == t.size / 2 ? 1 / s.sum : 2 / s.sum;
      const float db = float(
          10 * std::log10(std::max(1e-18, double(std::norm(s.output[bin])) *
                                              gain * gain)));
      result.db[std::size_t(frame - s.frames) * bins + bin] = db;
      result.maximumDb = std::max(result.maximumDb, db);
    }
  }
  s.frames = end;
  return result;
}
} // namespace drumfoundry::analysis
