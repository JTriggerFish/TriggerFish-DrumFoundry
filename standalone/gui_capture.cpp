#include "gui_capture.hpp"
namespace drumfoundry::standalone {
GuiCapture::GuiCapture(visage::ApplicationWindow &window, ui::Workbench &editor,
                       visage::Frame &shade, visage::Frame &settings)
    : window_(window), editor_(editor), shade_(shade), settings_(settings) {
  timer_.onTimerCallback() = [this] { Tick(); };
  timer_.startTimer(1500);
}
void GuiCapture::Tick() {
  if (!liveCaptured_) {
    const auto &shot = window_.takeScreenshot();
    if (shot.width() > 0 && shot.height() > 0) {
      shot.save("build/ui-live-smoke.png");
      liveCaptured_ = true;
    }
  }
  // Only the analysis screenshot needs a completed capture. Subsequent
  // settings/routing screenshots must not wait on new periodic smoke strikes.
  if (stage_ == 0 && !editor_.AnalysisReady()) {
    if (++attempts_ >= 40) {
      timer_.stopTimer();
      window_.window()->close();
    }
    return;
  }
  const auto &shot = window_.takeScreenshot();
  if (shot.width() > 0 && shot.height() > 0) {
    if (stage_ == 0) {
      shot.save("build/ui-smoke.png");
      shade_.setVisible(true);
      settings_.setVisible(true);
    } else if (stage_ == 2) {
      shot.save("build/ui-settings-smoke.png");
      settings_.setVisible(false);
      editor_.OpenRouting();
    } else if (stage_ == 4) {
      shot.save("build/ui-routing-smoke.png");
      captured_ = true;
    }
    ++stage_; // Allow a full drawn frame between view changes and capture.
  }
  if (captured_ || ++attempts_ >= 44) {
    timer_.stopTimer();
    window_.window()->close();
  }
}
} // namespace drumfoundry::standalone
