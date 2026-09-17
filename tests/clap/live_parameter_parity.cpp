#include "adapters/clap/plugin.hpp"
#include "editing/document.hpp"
#include "patch/modules.hpp"
#include <cmath>
#include <stdexcept>

// Every parameter advertised as live must produce the same next hit as loading
// its saved JSON from scratch. Detect incomplete live setters and hidden state.
void LiveParameterParity() {
  using namespace drumfoundry;
  using namespace clap_adapter;
  clap_host_t host{CLAP_VERSION, nullptr, "Parity", "TriggerFish", "", "1"};
  auto plugin = std::make_unique<Plugin>(&host);
  plugin->Init();
  for (unsigned preset = 0; preset < 7; ++preset) {
    if (preset < 6)
      plugin->SelectFactory(preset);
    else
      plugin->EditDocument(
          DefaultPatch(Topology(detail::Recipe::MembraneDrum).at("recipe")));
    editing::Document modules;
    modules.Load(plugin->EditableDocument());
    modules.SetModule(RimContactType, true);
    plugin->EditDocument(modules.JsonValue());
    const auto before = plugin->EditableDocument();
    Voice metadata(48000, before);
    const auto recipe = before.at("instrument").at("recipe").get<std::string>();
    std::size_t index = 0;
    for (const auto &description : metadata.Descriptors()) {
      const auto parameterIndex = index++;
      const auto key = description.at("key").get<std::string>();
      if (!IsLiveParameter(recipe, key))
        continue;
      const double low = description.at("minimum"),
                   high = description.at("maximum");
      const int scale = description.at("scale");
      double value = low + .37 * (high - low);
      if (scale >= 2)
        value = high;
      auto next = before;
      for (auto &node : next["instrument"]["nodes"])
        if (node["parameters"].contains(key))
          node["parameters"][key] = value;
      if (!ValidateLiveEdit(before, next))
        throw std::runtime_error("Live classification disagrees for " + key);
      Voice edited(48000, before), fresh(48000, next);
      if (!edited.StageParameter(parameterIndex, float(value)))
        throw std::runtime_error("Live staging rejected " + key);
      std::array<float, 512> silence{};
      edited.Process(silence.data(), silence.size()); // Finish gain/EQ ramps.
      edited.Reset();
      edited.Trigger(fresh.Event());
      fresh.Trigger(fresh.Event());
      std::array<float, 4096> a{}, b{};
      edited.Process(a.data(), a.size());
      fresh.Process(b.data(), b.size());
      double error = 0, energy = 0;
      for (std::size_t i = 0; i < a.size(); ++i) {
        error += std::pow(double(a[i]) - b[i], 2);
        energy += double(b[i]) * b[i];
      }
      if (error > 1e-12 + energy * 1e-9)
        throw std::runtime_error("Live/fresh render mismatch: " + recipe + "/" +
                                 key);
    }
  }
}
