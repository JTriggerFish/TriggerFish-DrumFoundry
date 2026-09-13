#include "measurement.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unsupported/Eigen/FFT>
namespace drumfoundry::decay_hold {
namespace {
constexpr unsigned Size = 4096, Bands = 24, Front = 120;
constexpr double Regions[][2] = {{0, .05},  {.05, .1}, {.1, .2}, {.2, .3},
                                 {.3, .45}, {1, 1.5},  {1.5, 2}, {2, 3},
                                 {3, 4},    {4, 5},    {5, 6}};
void Validate(const Cells &cells) {
  for (double x : cells)
    if (!std::isfinite(x) || x < 0)
      throw std::invalid_argument("Invalid hold-decay measurement");
}
} // namespace
void CheckCancelled(const Cancel &cancel) {
  if (cancel && cancel())
    throw std::runtime_error("Hold decay cancelled");
}
Cells Measure(const std::vector<float> &samples, unsigned rate,
              const Cancel &cancel) {
  if (rate < 8000 || rate > 384000 || samples.size() != 6u * rate)
    throw std::invalid_argument("Hold decay needs a six-second render");
  for (float x : samples)
    if (!std::isfinite(x))
      throw std::invalid_argument("Nonfinite hold-decay render");
  std::array<int, Size / 2 + 1> band;
  const double range = std::log(std::min(16000., .45 * rate) / 80);
  for (unsigned i = 0; i < band.size(); ++i) {
    const double f = double(i) * rate / Size;
    band[i] = f >= 80 && f < std::min(16000., .45 * rate)
                  ? int(Bands * std::log(f / 80) / range)
                  : -1;
  }
  std::array<double, Size> window;
  for (unsigned i = 0; i < Size; ++i)
    window[i] = .5 - .5 * std::cos(6.283185307179586 * i / (Size - 1));
  Cells power{};
  std::array<unsigned, 11> counts{};
  Eigen::FFT<double> fft;
  std::vector<double> input(Size);
  std::vector<std::complex<double>> output;
  for (unsigned centre = 0; centre < samples.size();
       centre += unsigned(std::lround(rate * .03))) {
    CheckCancelled(cancel);
    const double time = double(centre) / rate;
    for (unsigned region = 0; region < 11; ++region) {
      if (time < Regions[region][0] || time >= Regions[region][1])
        continue;
      for (unsigned i = 0; i < Size; ++i) {
        const auto index = int64_t(centre) + i - Size / 2;
        input[i] = index >= 0 && uint64_t(index) < samples.size()
                       ? samples[std::size_t(index)] * window[i]
                       : 0;
      }
      fft.fwd(output, input);
      ++counts[region];
      for (unsigned i = 1; i < band.size(); ++i)
        if (band[i] >= 0)
          power[region * Bands + band[i]] +=
              std::norm(output[i]) / (double(Size) * Size);
      break;
    }
  }
  for (unsigned i = 0; i < power.size(); ++i)
    power[i] /= std::max(1u, counts[i / Bands]);
  return power;
}
Targets::Targets(const Cells &baseline, const Cells &edited)
    : baseline_(baseline), edited_(edited) {
  Validate(baseline);
  Validate(edited);
  const double peak = *std::max_element(baseline.begin(), baseline.end());
  if (peak < 1e-18)
    throw std::invalid_argument("Hold decay needs an audible starting sound");
  floor_ = peak * 1e-7;
  for (unsigned i = 0; i < baseline.size(); ++i) {
    if (i >= Front && baseline[i] > peak * std::pow(10., -5.5))
      late_.push_back(i);
    if (i < Front && std::max(baseline[i], edited[i]) > peak * 1e-5)
      front_.push_back(i);
  }
  if (late_.size() < 8)
    throw std::invalid_argument("Not enough audible tail to hold");
}
Errors Targets::Compare(const Cells &measured) const {
  Validate(measured);
  Errors result;
  const auto difference = [&](unsigned i, const Cells &target) {
    return 10 * std::log10(std::max(floor_, measured[i]) /
                           std::max(floor_, target[i]));
  };
  for (auto i : late_)
    result.late.push_back(difference(i, baseline_));
  for (auto i : front_)
    result.front.push_back(difference(i, edited_));
  return result;
}
double Rms(const std::vector<double> &v) {
  double sum = 0;
  for (double x : v)
    sum += x * x;
  return std::sqrt(sum / std::max(std::size_t(1), v.size()));
}
} // namespace drumfoundry::decay_hold
