#include "adapters/clap/plugin.hpp"
#include "plugin_host.hpp"
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
  if (host.Value(100) != 2)
    return 1; // Standalone and CLAP share the official hi-hat startup voice.
  auto &stoppedPlugin = drumfoundry::clap_adapter::Plugin::Get(host.Api());
  stoppedPlugin.SelectFactory(0); // First queue test explicitly exercises kick EQ.
  for (const auto &p : drumfoundry::clap_adapter::DesignParameters())
    if (p.recipe == drumfoundry::detail::Recipe::Kick &&
        p.descriptor->key == "output_colour_gain") {
      for (int i = 0; i < 2200; ++i) {
        if (!stoppedPlugin.QueueEdit(p.id, i % 2 ? 3 : 4))
          return 1;
        host.Service();
      }
      stoppedPlugin.EndDesignGesture();
      host.Service();
      if (stoppedPlugin.Value(p.id) != 3)
        return 1;
    }
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
    auto &livePlugin = drumfoundry::clap_adapter::Plugin::Get(host.Api());
    auto liveDocument = livePlugin.EditableDocument();
    for (auto &node : liveDocument["instrument"]["nodes"])
      if (node["parameters"].contains("output_colour_gain"))
        node["parameters"]["output_colour_gain"] = 7.;
    if (!livePlugin.EditLiveDocument(liveDocument))
      return 1;
    {
      const char *key = preset == 0   ? "resonance_frequency_0"
                        : preset == 1 ? "fundamental_hz"
                                      : "body_tune";
      for (auto &node : liveDocument["instrument"]["nodes"])
        if (node["parameters"].contains(key))
          node["parameters"][key] = preset >= 2 ? 1.25 : 147.;
      if (!livePlugin.EditLiveDocument(liveDocument) ||
          livePlugin.RestartPending())
        return 1;
    }
    std::vector<std::pair<clap_id, double>> automated;
    for (const auto &p : drumfoundry::clap_adapter::DesignParameters())
      if (p.recipe == livePlugin.DesignRecipe()) {
        const auto &d = *p.descriptor;
        const double v = d.key == "hat_contact_enabled" ? 1.
                         : int(d.scale) >= 2
                             ? d.defaultValue
                             : d.minimum + .37 * (d.maximum - d.minimum);
        automated.emplace_back(p.id, v);
      }
    watching = true;
    for (int block = 0; block < 128; ++block) {
      if (block % 16 == 0)
        host.midi[0].Push({true, 0, 0, {0x90, 60, 100}});
      host.midi[1].Push({true, 0, 0,
                        {0xb9, 4, static_cast<uint8_t>(block % 16 ? 127 : 0)}});
      host.controls.Push({false, 105, -18.0, {}});
      // Exercise every advertised live setter in the actual audio callback.
      if (std::size_t(block) < automated.size())
        host.controls.Push(
            {false, automated[block].first, automated[block].second, {}});
      host.Process(output.data(), 128);
    }
    watching = false;
    if (preset >= 1) {
      // Adopt and process removed states from an already sounding body under
      // the allocation/free watcher, not just a geometry edit in silence.
      liveDocument = livePlugin.EditableDocument();
      const auto densityKey = preset == 1 ? "wire_density" : "field_satellite_density";
      for (auto &node : liveDocument["instrument"]["nodes"])
        if (node["parameters"].contains(densityKey))
          node["parameters"][densityKey] = 0.;
      if (!livePlugin.EditLiveDocument(liveDocument))
        return 1;
      watching = true;
      for (unsigned i = 0; i < 4; ++i)
        host.Process(output.data(), 128);
      watching = false;
    }
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
  std::array<float, 128> monitor{};
  const auto tapped =
      plugin.ReadOutput(monitor.data(), unsigned(monitor.size()));
  if (tapped.samples != 128 || tapped.rate != 48000 || tapped.dropped)
    return 1;
  for (unsigned i = 0; i < monitor.size(); ++i)
    if (monitor[i] != audition[2 * i])
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
