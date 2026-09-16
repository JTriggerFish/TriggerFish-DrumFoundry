#pragma once
#include <array>
#include <atomic>
#include <memory>
#include <stdexcept>

namespace drumfoundry::host {
// Single main-thread producer, single audio-thread consumer. Latest edit wins.
// Three slots cover one reading, one pending and one being prepared. Objects
// are allocated/reclaimed ONLY by the producer, never by Consume().
template <class T> class PreparedMailbox {
public:
  void Publish(std::unique_ptr<T> next) {
    for (int i = 0; i < 3; ++i) {
      if (slots_[i].busy.load(std::memory_order_acquire))
        continue;
      slots_[i].busy.store(true, std::memory_order_relaxed);
      slots_[i].value = std::move(next);
      const int replaced = pending_.exchange(i, std::memory_order_acq_rel);
      if (replaced >= 0)
        slots_[replaced].busy.store(false, std::memory_order_release);
      return;
    }
    throw std::logic_error("Prepared edit mailbox ownership violation");
  }
  template <class Apply> void Consume(Apply apply) noexcept {
    const int i = pending_.exchange(-1, std::memory_order_acq_rel);
    if (i < 0)
      return;
    apply(*slots_[i].value);
    slots_[i].busy.store(false, std::memory_order_release);
  }
  // Main-thread cancellation; an already consumed update finishes this block.
  void Cancel() noexcept {
    const int i = pending_.exchange(-1, std::memory_order_acq_rel);
    if (i >= 0)
      slots_[i].busy.store(false, std::memory_order_release);
  }

private:
  struct Slot {
    std::unique_ptr<T> value;
    std::atomic<bool> busy{};
  };
  std::array<Slot, 3> slots_;
  std::atomic<int> pending_{-1};
};
} // namespace drumfoundry::host
