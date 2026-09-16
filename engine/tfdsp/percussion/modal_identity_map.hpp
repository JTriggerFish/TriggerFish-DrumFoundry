#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace tfdsp::percussion {
// Fixed-capacity identity lookup. Preparation guarantees unique, nonzero IDs.
// No sorting, allocation or unbounded retry on the audio thread.
template <std::size_t Count> class ModalIdentityMap {
public:
  void Insert(std::uint32_t key, std::size_t value) noexcept {
    auto slot = Hash(key);
    for (std::size_t i = 0; i < keys_.size(); ++i) {
      if (!keys_[slot] || keys_[slot] == key) {
        keys_[slot] = key;
        values_[slot] = value;
        return;
      }
      slot = (slot + 1) % keys_.size();
    }
  }
  std::size_t Find(std::uint32_t key) const noexcept {
    auto slot = Hash(key);
    for (std::size_t i = 0; i < keys_.size(); ++i) {
      if (!keys_[slot])
        return Count;
      if (keys_[slot] == key)
        return values_[slot];
      slot = (slot + 1) % keys_.size();
    }
    return Count;
  }

private:
  static std::size_t Hash(std::uint32_t key) noexcept {
    key ^= key >> 16;
    key *= 0x7feb352du;
    key ^= key >> 15;
    return key % (2 * Count + 1);
  }
  std::array<std::uint32_t, 2 * Count + 1> keys_{};
  std::array<std::size_t, 2 * Count + 1> values_{};
};
} // namespace tfdsp::percussion
