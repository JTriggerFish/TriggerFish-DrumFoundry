#include "gui.hpp"
#include <algorithm>
#include <cstring>

namespace drumfoundry::clap_adapter {
namespace {
bool Supported(const char *api, bool floating) {
  return !floating && api && !std::strcmp(api, Editor::Api());
}
Editor *Get(const clap_plugin_t *p) { return Plugin::Get(p).editor.get(); }
} // namespace
const clap_plugin_gui_t GuiExtension{
    [](const clap_plugin_t *, const char *api, bool floating) {
      return Supported(api, floating);
    },
    [](const clap_plugin_t *, const char **api, bool *floating) {
      if (!api || !floating)
        return false;
      *api = Editor::Api();
      *floating = false;
      return true;
    },
    [](const clap_plugin_t *p, const char *api, bool floating) {
      if (!Supported(api, floating) || Get(p))
        return false;
      try {
        Plugin::Get(p).editor = std::make_unique<Editor>(Plugin::Get(p));
        return true;
      } catch (...) {
        return false;
      }
    },
    [](const clap_plugin_t *p) { Plugin::Get(p).editor.reset(); },
    [](const clap_plugin_t *, double) {
      return false;
    }, // Visage queries OS DPI.
    [](const clap_plugin_t *p, uint32_t *w, uint32_t *h) {
      if (!Get(p) || !w || !h)
        return false;
      *w = Get(p)->Width();
      *h = Get(p)->Height();
      return true;
    },
    [](const clap_plugin_t *p) { return bool(Get(p)); },
    [](const clap_plugin_t *p, clap_gui_resize_hints_t *hints) {
      if (!Get(p) || !hints)
        return false;
      *hints = {true, true, false, 0, 0};
      return true;
    },
    [](const clap_plugin_t *p, uint32_t *w, uint32_t *h) {
      if (!Get(p) || !w || !h)
        return false;
      *w = std::clamp(*w, 900u, 8192u);
      *h = std::clamp(*h, 600u, 8192u);
      return true;
    },
    [](const clap_plugin_t *p, uint32_t w, uint32_t h) {
      try {
        return Get(p) && Get(p)->Resize(w, h);
      } catch (...) {
        return false;
      }
    },
    [](const clap_plugin_t *p, const clap_window_t *parent) {
      try {
        return Get(p) && Get(p)->Parent(parent);
      } catch (...) {
        return false;
      }
    },
    [](const clap_plugin_t *, const clap_window_t *) { return false; },
    [](const clap_plugin_t *, const char *) {},
    [](const clap_plugin_t *p) { return Get(p) && Get(p)->Show(true); },
    [](const clap_plugin_t *p) { return Get(p) && Get(p)->Show(false); }};
const clap_plugin_posix_fd_support_t FdExtension{
    [](const clap_plugin_t *p, int fd, clap_posix_fd_flags_t) {
      if (Get(p))
        Get(p)->PollFd(fd);
    }};
} // namespace drumfoundry::clap_adapter
