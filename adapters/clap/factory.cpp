#include "plugin.hpp"
#ifdef DRUMFOUNDRY_UI
#include "gui.hpp"
#endif
#include <cstring>

namespace drumfoundry::clap_adapter {
Plugin::~Plugin() {
#ifdef DRUMFOUNDRY_UI
  editor.reset(); // Stop callbacks before the plugin's control storage dies.
#endif
}
namespace {
const char *const Features[]{CLAP_PLUGIN_FEATURE_INSTRUMENT,
                             CLAP_PLUGIN_FEATURE_DRUM,
                             CLAP_PLUGIN_FEATURE_STEREO, nullptr};
}
const clap_plugin_descriptor_t Descriptor{
    CLAP_VERSION,
    PluginId,
    "TriggerFish DrumFoundry",
    "TriggerFish",
    "https://github.com/JTriggerFish/TriggerFish-DrumFoundry",
    "",
    "",
    "0.1.0",
    "Native modular percussion synthesizer — headless development shell",
    Features};

Plugin::Plugin(const clap_host_t *host) : host_(host) {
  api.desc = &Descriptor;
  api.plugin_data = this;
  api.init = [](const clap_plugin_t *p) noexcept {
    try {
      return Get(p).Init();
    } catch (...) {
      return false;
    }
  };
  api.destroy = [](const clap_plugin_t *p) noexcept { delete &Get(p); };
  api.activate = [](const clap_plugin_t *p, double rate, uint32_t min,
                    uint32_t max) noexcept {
    try {
      return Get(p).Activate(rate, min, max);
    } catch (...) {
      return false;
    }
  };
  api.deactivate = [](const clap_plugin_t *p) noexcept { Get(p).Deactivate(); };
  api.start_processing = [](const clap_plugin_t *p) noexcept {
    auto &self = Get(p);
    if (!self.active || self.processing)
      return false;
    self.processing = true;
    return true;
  };
  api.stop_processing = [](const clap_plugin_t *p) noexcept {
    Get(p).processing = false;
  };
  api.reset = [](const clap_plugin_t *p) noexcept { Get(p).Reset(); };
  api.process = [](const clap_plugin_t *p,
                   const clap_process_t *process) noexcept {
    return Get(p).Process(process);
  };
  api.get_extension = [](const clap_plugin_t *p, const char *id) noexcept {
    return Get(p).Extension(id);
  };
  api.on_main_thread = [](const clap_plugin_t *p) noexcept {
    Get(p).OnMainThread();
  };
}
} // namespace drumfoundry::clap_adapter

namespace {
const clap_plugin_factory_t Factory{
    [](const clap_plugin_factory_t *) -> uint32_t { return 1; },
    [](const clap_plugin_factory_t *,
       uint32_t index) -> const clap_plugin_descriptor_t * {
      return index == 0 ? &drumfoundry::clap_adapter::Descriptor : nullptr;
    },
    [](const clap_plugin_factory_t *, const clap_host_t *host,
       const char *id) -> const clap_plugin_t * {
      if (!host || !id || !clap_version_is_compatible(host->clap_version) ||
          std::strcmp(id, drumfoundry::clap_adapter::PluginId))
        return nullptr;
      try {
        return &(new drumfoundry::clap_adapter::Plugin(host))->api;
      } catch (...) {
        return nullptr;
      }
    }};
} // namespace
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry{
    CLAP_VERSION, [](const char *) { return true; }, []() {},
    [](const char *id) -> const void * {
      return id && !std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? &Factory
                                                            : nullptr;
    }};
