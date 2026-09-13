#include "gui.hpp"
#include <cstring>
#include <stdexcept>

namespace drumfoundry::clap_adapter {
namespace {
ui::Bridge Connect(Plugin &plugin) {
  ui::Bridge bridge;
  bridge.value = [&plugin](unsigned id) { return plugin.Value(id); };
  bridge.velocity = [&plugin] { return plugin.PreviewStrength(); };
  bridge.setVelocity = [&plugin](double v) { plugin.SetPreviewStrength(v); };
  bridge.sampleRate = [&plugin] { return plugin.AuditionRate(); };
  bridge.play = [&plugin](auto pcm, unsigned rate, double gain) {
    if (!plugin.Audition(std::move(pcm), rate, gain))
      throw std::runtime_error(
          "Audio is stopped, still preparing, or audition queue is full");
  };
  bridge.document = [&plugin] { return plugin.EditableDocument(); };
  bridge.presentation = [&plugin](const auto &ref, const auto &analysis) {
    plugin.EditPresentation(ref, analysis);
  };
  bridge.service = [&plugin] { plugin.PrepareEditorPreset(); };
  bridge.applyDocument = [&plugin](const auto &document) {
    plugin.EditDocument(document);
  };
  bridge.revision = [&plugin] { return plugin.DocumentRevision(); };
  bridge.change = [&plugin](unsigned id, double value) {
    if (!plugin.QueueEdit(id, value))
      throw std::runtime_error("Editor control queue full or invalid value");
  };
  bridge.strike = [&plugin](float v, float x) {
    if (!plugin.QueueStrike(v, x))
      throw std::runtime_error("Editor strike queue full");
  };
  bridge.stop = [&plugin] {
    if (!plugin.QueuePanic())
      throw std::runtime_error("Editor control queue full");
  };
  bridge.status = [&plugin] {
    return plugin.EditorErrors()
               ? "Editor event delivery errors: " +
                     std::to_string(plugin.EditorErrors())
               : "CLAP host audio / MIDI — device settings belong to your host";
  };
  return bridge;
}
} // namespace
bool Plugin::Audition(std::shared_ptr<const std::vector<float>> pcm,
                      unsigned rate, double gain) {
  if (!rate || rate != auditionRate_ ||
      !audition_.Submit(std::move(pcm), rate, gain))
    return false;
  if (host_->request_process)
    host_->request_process(host_);
  return true;
}
const char *Editor::Api() {
#if defined(_WIN32)
  return CLAP_WINDOW_API_WIN32;
#elif defined(__APPLE__)
  return CLAP_WINDOW_API_COCOA;
#else
  return CLAP_WINDOW_API_X11;
#endif
}
Editor::Editor(Plugin &plugin) : plugin_(plugin), content_(Connect(plugin)) {
  app_.setMinimumDimensions(1000, 640);
  app_.addChild(&content_);
  app_.onResize() = [this] { content_.setBounds(app_.localBounds()); };
  app_.setNativeBounds(0, 0, width_, height_);
}
Editor::~Editor() {
  if (registeredFd_ >= 0 && fds_)
    fds_->unregister_fd(plugin_.Host(), registeredFd_);
  app_.removeFromWindow();
  window_.reset();
}
bool Editor::Parent(const clap_window_t *parent) {
  if (!parent || !parent->api || std::strcmp(parent->api, Api()) || window_)
    return false;
  void *handle = parent->ptr;
#if defined(__linux__)
  handle = reinterpret_cast<void *>(static_cast<uintptr_t>(parent->x11));
  const auto *host = plugin_.Host();
  fds_ = host->get_extension
             ? static_cast<const clap_host_posix_fd_support_t *>(
                   host->get_extension(host, CLAP_EXT_POSIX_FD_SUPPORT))
             : nullptr;
  if (!fds_ || !fds_->register_fd || !fds_->unregister_fd)
    return false;
#endif
  if (!handle)
    return false;
  window_ = visage::createPluginWindow(visage::Dimension::nativePixels(width_),
                                       visage::Dimension::nativePixels(height_),
                                       handle);
  if (!window_)
    return false;
#if defined(__linux__)
  const int fd = window_->posixFd();
  if (!fds_->register_fd(host, fd, CLAP_POSIX_FD_READ)) {
    window_.reset();
    return false;
  }
  registeredFd_ = fd;
#endif
  app_.addToWindow(window_.get());
  return true;
}
bool Editor::Resize(uint32_t width, uint32_t height) {
  if (width < 1000 || height < 640 || width > 8192 || height > 8192)
    return false;
  width_ = width;
  height_ = height;
  float scale = 1;
#ifdef __APPLE__
  // CLAP Cocoa sizes are logical points; Visage native bounds are pixels.
  if (window_)
    scale = window_->dpiScale();
#endif
  const int nativeWidth = int(width * scale),
            nativeHeight = int(height * scale);
  app_.setNativeBounds(0, 0, nativeWidth, nativeHeight);
  if (window_)
    window_->setNativeWindowSize(nativeWidth, nativeHeight);
  return true;
}
bool Editor::Show(bool visible) {
  if (!window_)
    return false;
  if (visible)
    window_->show();
  else
    window_->hide();
  return true;
}
void Editor::PollFd(int fd) {
  if (window_ && fd == registeredFd_)
    window_->processPluginFdEvents();
}
} // namespace drumfoundry::clap_adapter
