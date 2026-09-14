#pragma once
#include "ui/workbench.hpp"
#include <visage/app.h>
namespace drumfoundry::standalone {
// Device-free smoke capture sequencing, separate from application setup.
class GuiCapture {
public:
  GuiCapture(visage::ApplicationWindow &, ui::Workbench &, visage::Frame &shade,
             visage::Frame &settings);
  ~GuiCapture() { timer_.stopTimer(); }
  bool Complete() const { return captured_; }

private:
  void Tick();
  void Resize(unsigned width, unsigned height);
  visage::ApplicationWindow &window_;
  ui::Workbench &editor_;
  visage::Frame &shade_, &settings_;
  visage::EventTimer timer_;
  unsigned attempts_{}, stage_{};
  bool captured_{}, liveCaptured_{};
};
} // namespace drumfoundry::standalone
