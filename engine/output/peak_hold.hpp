#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace drumfoundry::output {
// Sliding maximum with amortized constant work. Allocation is preparation-only.
class PeakHold {
public:
  void Prepare(std::size_t window) {
    if (window == std::numeric_limits<std::size_t>::max())
      throw std::invalid_argument("Peak hold window is too large");
    window_ = std::max<std::size_t>(window, 1);
    values_.resize(window_ + 1);
    times_.resize(window_ + 1);
    Reset();
  }
  void Reset() noexcept {
    head_ = tail_ = 0;
    time_ = 0;
  }
  double Process(double value) noexcept {
    while (head_ != tail_ && time_ - times_[head_] >= window_)
      head_ = Next(head_);
    while (head_ != tail_) {
      const auto previous = (tail_ + values_.size() - 1) % values_.size();
      if (values_[previous] > value)
        break;
      tail_ = previous;
    }
    values_[tail_] = value;
    times_[tail_] = time_++;
    tail_ = Next(tail_);
    return values_[head_];
  }

private:
  std::size_t Next(std::size_t i) const noexcept {
    return (i + 1) % values_.size();
  }
  std::vector<double> values_;
  std::vector<std::uint64_t> times_;
  std::size_t window_{1}, head_{}, tail_{};
  std::uint64_t time_{};
};

// Positive FIR weights preserve the limiter's gain bound. Direct summation
// avoids cancellation of tiny gains after extreme overload; lengths are short.
class GainAverage {
public:
  void Prepare(std::size_t length) {
    if (length == 0)
      throw std::invalid_argument("Gain average requires a positive length");
    values_.resize(length);
    Reset();
  }
  void Reset() noexcept {
    std::fill(values_.begin(), values_.end(), 1.);
    write_ = 0;
  }
  double Process(double gain) noexcept {
    values_[write_] = gain;
    write_ = (write_ + 1) % values_.size();
    double sum = 0;
    for (double value : values_)
      sum += value;
    return sum / values_.size();
  }

private:
  std::vector<double> values_;
  std::size_t write_{};
};
} // namespace drumfoundry::output
