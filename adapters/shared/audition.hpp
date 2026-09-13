#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>
namespace drumfoundry::host {
// Three immutable slots: main thread owns all allocation and destruction.
// The audio consumer changes only atomic ownership flags and its read cursor.
// Publishing while all slots are occupied fails explicitly, never blocks audio.
class Audition {
public:
  bool Submit(std::shared_ptr<const std::vector<float>> pcm, unsigned rate,
              double gain) {
    if (!pcm || pcm->empty() || rate < 8000 || rate > 384000 ||
        !std::isfinite(gain) || gain < 0)
      return false;
    for (auto &slot : slots_) {
      if (slot.state.load(std::memory_order_acquire) != Free)
        continue;
      slot.pcm = std::move(pcm);
      slot.rate = rate;
      slot.gain = gain;
      slot.sequence = ++sequence_;
      slot.state.store(Ready, std::memory_order_release);
      return true;
    }
    return false;
  }
  // Audio-thread block boundary; newest queued audition replaces older ones.
  bool Begin(unsigned rate) noexcept {
    Slot *next = nullptr;
    for (auto &slot : slots_)
      if (slot.state.load(std::memory_order_acquire) == Ready &&
          (!next || slot.sequence > next->sequence))
        next = &slot;
    if (!next)
      return false;
    if (current_)
      current_->state.store(Free, std::memory_order_release);
    current_ = next;
    cursor_ = 0;
    current_->state.store(Playing, std::memory_order_relaxed);
    for (auto &slot : slots_)
      if (&slot != current_ &&
          slot.state.load(std::memory_order_acquire) == Ready &&
          slot.sequence < current_->sequence)
        slot.state.store(Free, std::memory_order_release);
    if (current_->rate != rate)
      Stop();
    return true;
  }
  bool Active() const noexcept { return current_ != nullptr; }
  float Next() noexcept {
    if (!current_)
      return 0;
    const float sample = float((*current_->pcm)[cursor_++] * current_->gain);
    if (cursor_ == current_->pcm->size()) {
      current_->state.store(Free, std::memory_order_release);
      current_ = nullptr;
    }
    return sample;
  }
  // Audio thread, or main thread after the audio callback has joined.
  void Stop() noexcept {
    if (current_)
      current_->state.store(Free, std::memory_order_release);
    current_ = nullptr;
    for (auto &slot : slots_)
      if (slot.state.load(std::memory_order_acquire) == Ready)
        slot.state.store(Free, std::memory_order_release);
  }

private:
  enum State { Free, Ready, Playing };
  struct Slot {
    std::atomic<State> state{Free};
    std::shared_ptr<const std::vector<float>> pcm;
    unsigned rate{};
    double gain{};
    uint64_t sequence{};
  };
  std::array<Slot, 3> slots_;
  Slot *current_{};
  std::size_t cursor_{};
  uint64_t sequence_{}; // Main-thread producer only.
};
} // namespace drumfoundry::host
