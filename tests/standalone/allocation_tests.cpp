#include "plugin_host.hpp"
#ifdef DRUMFOUNDRY_UI
#include "adapters/clap/plugin.hpp"
#endif
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
bool watching{};
std::size_t allocations{};
std::size_t deallocations{};
} // namespace
// The actual CLAP adapter is statically linked, so its ordinary heap
// allocations (including strike/parameter handling) are observed, not just host
// allocations.
void *operator new(std::size_t size) {
  if (watching)
    ++allocations;
  if (auto *p = std::malloc(size ? size : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept {
  if (watching && p)
    ++deallocations;
  std::free(p);
}
void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete(void *p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t) noexcept { ::operator delete(p); }

int main() {
  drumfoundry::standalone::PluginHost host;
  // Restart retains pending parameter edits, but clears old manual/MIDI notes.
  host.Prepare(48000, 128);
  host.controls.Push({false, 105, -20, {}});
  host.controls.Push({true, 0, 0, {0x90, 60, 100}});
  host.midi[31].Push({true, 0, 0, {0x90, 60, 100}});
  host.Stop();
  if (host.Value(105) != -20)
    return 1;
  host.Prepare(48000, 128);
  std::array<float, 256> silent{};
  host.Process(silent.data(), 128);
  for (float sample : silent)
    if (sample != 0)
      return 1;
  // A flood from earlier queues cannot defer the last MIDI port a whole block.
  for (int i = 0; i < 255; ++i) {
    host.controls.Push({false, 105, -20, {}});
    host.midi[0].Push({true, 0, 0, {0xb0, 1, 0}});
  }
  host.midi[31].Push({true, 0, 0, {0x90, 60, 100}});
  host.Process(silent.data(), 128);
  drumfoundry::standalone::Event pending;
  if (host.midi[31].Pop(pending))
    return 1;
  host.Stop();
  for (int preset = 0; preset < 6; ++preset) {
    host.SetStopped(100, preset);
    host.Prepare(48000, 128);
    std::array<float, 256> output{};
    watching = true;
    for (int block = 0; block < 128; ++block) {
      if (block % 16 == 0)
        host.midi[0].Push({true, 0, 0, {0x90, 60, 100}});
      host.controls.Push({false, 105, -18.0, {}});
      host.Process(output.data(), 128);
    }
    watching = false;
    host.Stop();
  }
  if (allocations)
    std::cerr << "Audio path allocated " << allocations << " times\n";
#ifdef DRUMFOUNDRY_UI
  host.SetStopped(105, 0);
  host.SetStopped(106, 0);
  host.Prepare(48000, 128);
  auto &plugin = drumfoundry::clap_adapter::Plugin::Get(host.Api());
  const auto pcm = std::make_shared<const std::vector<float>>(512, .25f);
  if (!plugin.Audition(pcm, 48000, 2) || plugin.Audition(pcm, 44100, 1))
    return 1;
  std::array<float, 256> audition{};
  watching = true;
  host.Process(audition.data(), 128);
  watching = false;
  for (float sample : audition)
    if (sample != .5f)
      return 1;
  host.controls.Push({true, 0, 0, {0xb0, 120, 0}});
  watching = true;
  host.Process(audition.data(), 128);
  watching = false;
  for (float sample : audition)
    if (sample != 0)
      return 1;
  host.Stop();
#endif
  if (deallocations)
    std::cerr << "Audio path freed memory " << deallocations << " times\n";
  return allocations || deallocations || host.failures ? 1 : 0;
}
