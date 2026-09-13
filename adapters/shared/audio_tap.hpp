#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace drumfoundry::host {
struct TapRead {
  unsigned samples{}, rate{}, generation{}, dropped{};
  unsigned firstStrike{~0u}, lastStrike{~0u};
};
// Single audio producer / main-thread reader. A stalled display drops monitor
// data, never blocks audio or overwrites data being read. Reset only while
// stopped.
class AudioTap {
public:
  static constexpr unsigned Capacity = 32768;
  void Reset(unsigned rate) noexcept {
    read_ = write_ = dropped_ = 0;
    lastDropped_ = 0;
    rate_ = rate;
    ++generation_;
  }
  void Push(const float *samples, unsigned count,
            bool strike = false) noexcept {
    if (!count)
      return;
    const unsigned w = write_.load(std::memory_order_relaxed);
    const unsigned r = read_.load(std::memory_order_acquire);
    if (count > Capacity || count > Capacity - (w - r)) {
      dropped_.fetch_add(1, std::memory_order_relaxed);
      return;
    }
    const unsigned offset = w & (Capacity - 1);
    const unsigned first = std::min(count, Capacity - offset);
    std::memcpy(data_.data() + offset, samples, first * sizeof(float));
    std::memcpy(data_.data(), samples + first, (count - first) * sizeof(float));
    std::fill_n(strikes_.data() + offset, first, uint8_t(0));
    std::fill_n(strikes_.data(), count - first, uint8_t(0));
    strikes_[offset] = strike;
    write_.store(w + count, std::memory_order_release);
  }
  TapRead Read(float *destination, unsigned maximum) noexcept {
    const unsigned r = read_.load(std::memory_order_relaxed);
    const unsigned w = write_.load(std::memory_order_acquire);
    const unsigned dropped = dropped_.load(std::memory_order_relaxed);
    unsigned count = std::min(maximum, w - r);
    unsigned firstStrike = ~0u, lastStrike = ~0u;
    if (dropped != lastDropped_)
      count = 0; // Discard stale backlog after a paused/closed editor.
    lastDropped_ = dropped;
    if (count) {
      const unsigned offset = (w - count) & (Capacity - 1);
      const unsigned first = std::min(count, Capacity - offset);
      std::memcpy(destination, data_.data() + offset, first * sizeof(float));
      std::memcpy(destination + first, data_.data(),
                  (count - first) * sizeof(float));
      for (unsigned i = 0; i < count; ++i)
        if (strikes_[(offset + i) & (Capacity - 1)]) {
          if (firstStrike == ~0u)
            firstStrike = i;
          lastStrike = i;
        }
    }
    read_.store(w, std::memory_order_release);
    return {count, rate_, generation_, dropped, firstStrike, lastStrike};
  }

private:
  static_assert(std::atomic<unsigned>::is_always_lock_free);
  std::array<float, Capacity> data_{};
  std::array<uint8_t, Capacity> strikes_{};
  alignas(64) std::atomic<unsigned> read_{};
  alignas(64) std::atomic<unsigned> write_{};
  std::atomic<unsigned> dropped_{};
  unsigned rate_{}, generation_{}, lastDropped_{}; // Main thread only.
};
} // namespace drumfoundry::host
