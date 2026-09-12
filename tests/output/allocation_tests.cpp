#include "output/limiter.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
bool watching{};
std::size_t allocations{};
} // namespace
// The output library is statically linked here, so ordinary operator-new calls
// from its prepared processing path are visible to this regression test.
void *operator new(std::size_t count) {
  if (watching)
    ++allocations;
  if (auto *p = std::malloc(count ? count : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t count) { return ::operator new(count); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

int main() {
  drumfoundry::output::Limiter limiter;
  limiter.Prepare(48000, 2);
  std::array<float, 512> left{}, right{};
  left.fill(4);
  right.fill(-2);
  float *channels[]{left.data(), right.data()};
  watching = true;
  for (int i = 0; i < 100; ++i) {
    limiter.Process(channels, left.size());
    const auto status = limiter.Status();
    if (!status.enabled)
      return 2;
    limiter.ClearMeters();
    limiter.Reset();
  }
  watching = false;
  if (allocations)
    std::cerr << "Audio path allocated " << allocations << " times\n";
  return allocations ? 1 : 0;
}
