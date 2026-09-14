#include "adapters/clap/gui.hpp"
#include "gui.hpp"
#include <stdexcept>

namespace drumfoundry::standalone {
// Real embedded CLAP lifecycle, without opening an audio or MIDI device.
void PluginGuiSmoke(PluginHost &host) {
  const auto *plugin = host.Api();
  const auto *gui = static_cast<const clap_plugin_gui_t *>(
      plugin->get_extension(plugin, CLAP_EXT_GUI));
  if (!gui)
    throw std::runtime_error("CLAP GUI extension missing");
  const char *api{};
  bool floating{};
  if (!gui->get_preferred_api(plugin, &api, &floating) || floating ||
      !gui->create(plugin, api, false))
    throw std::runtime_error("CLAP editor creation failed");
  visage::ApplicationWindow window;
  window.setTitle("DrumFoundry — CLAP embedding test");
  window.show(1440, 900);
  clap_window_t parent{};
  parent.api = api;
  parent.ptr = window.window()->nativeHandle();
  bool passed = false;
  try {
    if (!gui->set_parent(plugin, &parent) ||
        !gui->set_size(plugin, 1200, 800) || !gui->show(plugin))
      throw std::runtime_error("CLAP parenting/resizing/show failed");
    clap_gui_resize_hints_t hints{};
    uint32_t adjustedWidth = 600, adjustedHeight = 400;
    if (!gui->can_resize(plugin) || !gui->get_resize_hints(plugin, &hints) ||
        !hints.can_resize_horizontally || !hints.can_resize_vertically ||
        hints.preserve_aspect_ratio ||
        !gui->adjust_size(plugin, &adjustedWidth, &adjustedHeight) ||
        adjustedWidth != 900 || adjustedHeight != 600 ||
        gui->set_size(plugin, 899, 600))
      throw std::runtime_error("CLAP resize contract failed");
    visage::EventTimer timer;
    unsigned attempts = 0, stage = 0;
    timer.onTimerCallback() = [&] {
      auto &editor = *clap_adapter::Plugin::Get(plugin).editor;
      const auto shot = editor.Screenshot();
      if (shot.width() > 0) {
        if (stage == 0) {
          shot.save("build/clap-ui-smoke.png");
          if (!gui->set_size(plugin, 900, 600))
            throw std::runtime_error("CLAP small editor resize failed");
          ++stage;
        } else if (shot.width() != 1200 || shot.height() != 800) {
          uint32_t w{}, h{};
          if (!gui->get_size(plugin, &w, &h) || w != 900 || h != 600)
            throw std::runtime_error("CLAP resized dimensions disagree");
          shot.save("build/clap-ui-small-smoke.png");
          passed = true;
        }
      }
      if (passed || ++attempts == 10) {
        timer.stopTimer();
        gui->hide(plugin);
        window.window()->close();
      }
    };
    timer.startTimer(1000);
    window.runEventLoop();
    gui->destroy(plugin);
  } catch (...) {
    gui->destroy(plugin);
    throw;
  }
  if (!passed)
    throw std::runtime_error("CLAP editor did not produce a frame");
}
} // namespace drumfoundry::standalone
