#pragma once
#include "plugin.hpp"
#include "ui/workbench.hpp"
#include <visage/app.h>

namespace drumfoundry::clap_adapter {
// Window/resources belong to the host main thread; no audio-thread UI access.
class Editor {
public:
  explicit Editor(Plugin &);
  ~Editor();
  bool Parent(const clap_window_t *);
  bool Resize(uint32_t width, uint32_t height);
  bool Show(bool visible);
  void PollFd(int fd);
  const visage::Screenshot &Screenshot() { return app_.takeScreenshot(); }
  uint32_t Width() const { return width_; }
  uint32_t Height() const { return height_; }
  static const char *Api();

private:
  Plugin &plugin_;
  visage::ApplicationEditor app_;
  ui::Workbench content_;
  std::unique_ptr<visage::Window> window_;
  const clap_host_posix_fd_support_t *fds_{};
  int registeredFd_{-1};
  uint32_t width_{1440}, height_{900};
};
} // namespace drumfoundry::clap_adapter
