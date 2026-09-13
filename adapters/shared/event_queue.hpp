#pragma once
#include <array>
#include <atomic>
#include <clap/clap.h>

namespace drumfoundry::host {
// One producer per MIDI port (or UI), one audio consumer. Bounded, no
// allocation or locks in either callback; overload is reported, never silently
// overwritten.
struct Event {
  bool midi{};
  clap_id parameter{};
  double value{};
  std::array<uint8_t, 3> bytes{};
  float strikeVelocity{}; // Native pad gestures retain continuous strength.
};
template <std::size_t Size = 256> class EventQueue {
public:
  bool Push(Event e) noexcept {
    const auto write = write_.load(std::memory_order_relaxed);
    const auto next = (write + 1) % Size;
    if (next == read_.load(std::memory_order_acquire)) {
      ++dropped_;
      return false;
    }
    data_[write] = e;
    write_.store(next, std::memory_order_release);
    return true;
  }
  bool Pop(Event &e) noexcept {
    const auto read = read_.load(std::memory_order_relaxed);
    if (read == write_.load(std::memory_order_acquire))
      return false;
    e = data_[read];
    read_.store((read + 1) % Size, std::memory_order_release);
    return true;
  }
  unsigned Dropped() const noexcept { return dropped_.load(); }
  // Consumer only (or main thread after audio has joined). A producer may
  // continue publishing: discard only the snapshot, never chase incoming data.
  void DiscardPending() noexcept {
    read_.store(write_.load(std::memory_order_acquire),
                std::memory_order_release);
  }

private:
  static_assert(Size > 1);
  std::array<Event, Size> data_{};
  std::atomic<std::size_t> read_{}, write_{};
  std::atomic<unsigned> dropped_{};
};
} // namespace drumfoundry::host
