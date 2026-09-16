#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace tfdsp::percussion {
// Short linear transitions for live gain/bypass edits, not an envelope or
// additional instrument parameter. Static renders retain their exact gains.
class LiveGain {
public:
  void Reset(float value) noexcept {
    current_ = target_ = value;
    left_ = 0;
  }
  void Target(float value, float sampleRate) noexcept {
    if (value == target_)
      return;
    target_ = value;
    left_ = std::max<std::size_t>(1, std::size_t(.005f * sampleRate));
    step_ = (target_ - current_) / float(left_);
  }
  float Next() noexcept {
    if (left_) {
      current_ += step_;
      if (!--left_)
        current_ = target_;
    }
    return current_;
  }
  bool Moving() const noexcept { return left_ != 0; }

private:
  float current_{}, target_{}, step_{};
  std::size_t left_{};
};
} // namespace tfdsp::percussion
