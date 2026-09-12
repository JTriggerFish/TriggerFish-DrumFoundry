#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace drumfoundry::output {
// Four-phase Blackman-windowed sinc estimate. Detector history/delay stays
// inside the audio lookahead. This is an estimate, not a true-peak
// certification.
class PeakDetector {
public:
  void Prepare(std::size_t radius) {
    if (radius < 1 || radius > 32)
      throw std::invalid_argument(
          "Peak detector radius must be between 1 and 32");
    length_ = 2 * radius + 1;
    constexpr double pi = 3.14159265358979323846;
    for (std::size_t phase = 0; phase < 3; ++phase) {
      double sum = 0;
      for (std::size_t tap = 0; tap < length_; ++tap) {
        const double x = double(tap) - radius + .25 * (phase + 1);
        const double window = std::abs(x) < radius
                                  ? .42 + .5 * std::cos(pi * x / radius) +
                                        .08 * std::cos(2 * pi * x / radius)
                                  : 0;
        coefficients_[phase][tap] = std::sin(pi * x) / (pi * x) * window;
        sum += coefficients_[phase][tap];
      }
      for (std::size_t tap = 0; tap < length_; ++tap)
        coefficients_[phase][tap] /= sum;
    }
    Reset();
  }
  void Reset() noexcept {
    history_.fill(0);
    write_ = 0;
  }
  double Process(double value) noexcept {
    history_[write_] = value;
    double peak = std::abs(value);
    for (const auto &phase : coefficients_) {
      double sum = 0;
      auto index = write_;
      for (std::size_t tap = 0; tap < length_; ++tap) {
        sum += phase[tap] * history_[index];
        index = index ? index - 1 : length_ - 1;
      }
      peak = std::max(peak, std::abs(sum));
    }
    write_ = (write_ + 1) % length_;
    return peak;
  }

private:
  std::array<std::array<double, 65>, 3> coefficients_{};
  std::array<double, 65> history_{};
  std::size_t length_{65}, write_{};
};
} // namespace drumfoundry::output
