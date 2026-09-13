#include "live_spectrum.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <unsupported/Eigen/FFT>
namespace drumfoundry::analysis {
struct LiveSpectrum::Impl {
  std::array<float, Size> history{}, window{};
  std::vector<float> input = std::vector<float>(Size);
  std::vector<float> db = std::vector<float>(Size / 2 + 1, -120);
  std::vector<std::complex<float>> bins;
  Eigen::FFT<float> fft;
  unsigned cursor{}, rate{};
  Impl() {
    for (unsigned i = 0; i < Size; ++i)
      window[i] = float(.5 - .5 * std::cos(6.283185307179586 * i / Size));
  }
};
LiveSpectrum::LiveSpectrum() : impl_(std::make_unique<Impl>()) {}
LiveSpectrum::~LiveSpectrum() = default;
void LiveSpectrum::Reset(unsigned rate) {
  if (rate && (rate < 8000 || rate > 384000))
    throw std::invalid_argument("Invalid live spectrum sample rate");
  impl_->history.fill(0);
  std::fill(impl_->db.begin(), impl_->db.end(), -120);
  impl_->cursor = 0;
  impl_->rate = rate;
}
void LiveSpectrum::Update(const float *samples, unsigned count) {
  if (!count || !impl_->rate)
    return;
  auto &s = *impl_;
  const unsigned skip = count > Size ? count - Size : 0;
  for (unsigned i = skip; i < count; ++i)
    if (!std::isfinite(samples[i]))
      throw std::runtime_error("Live output contains nonfinite samples");
  for (unsigned i = skip; i < count; ++i) {
    s.history[s.cursor] = samples[i];
    s.cursor = (s.cursor + 1) % Size;
  }
  for (unsigned i = 0; i < Size; ++i)
    s.input[i] = s.history[(s.cursor + i) % Size] * s.window[i];
  s.fft.fwd(s.bins, s.input);
  for (unsigned i = 0; i <= Size / 2; ++i) {
    const double gain = i == 0 || i == Size / 2 ? 2. / Size : 4. / Size;
    s.db[i] =
        float(10 * std::log10(std::max(1e-12, double(std::norm(s.bins[i])) *
                                                  gain * gain)));
  }
}
const std::vector<float> &LiveSpectrum::Decibels() const { return impl_->db; }
unsigned LiveSpectrum::Rate() const { return impl_->rate; }
} // namespace drumfoundry::analysis
