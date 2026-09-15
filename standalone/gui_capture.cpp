#include "gui_capture.hpp"
namespace drumfoundry::standalone {
void GuiCapture::Resize(unsigned width, unsigned height) {
  const float scale = window_.window()->dpiScale();
  window_.window()->setNativeWindowSize(int(width * scale),
                                        int(height * scale));
}
GuiCapture::GuiCapture(visage::ApplicationWindow &window,
                       ui::Workbench &editor, visage::Frame &shade,
                       visage::Frame &settings)
    : window_(window), editor_(editor), shade_(shade), settings_(settings) {
  timer_.onTimerCallback() = [this] { Tick(); };
  timer_.startTimer(1500);
}
void GuiCapture::Tick() {
  if (!liveCaptured_) {
    const auto shot = window_.takeScreenshot();
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
  const auto shot = window_.takeScreenshot();
  if (shot.width() > 0 && shot.height() > 0) {
    if (stage_ == 0) {
      shot.save("build/ui-smoke.png");
      Resize(900, 600);
      editor_.SetControlWidth(340);
    } else if (stage_ == 2) {
      shot.save("build/ui-small-smoke.png");
      for (auto *child : editor_.children())
        if (auto *panel = dynamic_cast<ui::ParameterPanel *>(child))
          panel->setYPosition(180);
    } else if (stage_ == 4) {
      shot.save("build/ui-scrolled-groups-smoke.png");
      for (auto *child : editor_.children())
        if (auto *panel = dynamic_cast<ui::ParameterPanel *>(child))
          panel->setYPosition(0);
      editor_.SetVisualPanels(false, false);
    } else if (stage_ == 6) {
      shot.save("build/ui-compact-smoke.png");
      editor_.SetTextSize(1);
      editor_.SetVisualPanels(true, true);
    } else if (stage_ == 8) {
      shot.save("build/ui-medium-text-smoke.png");
      editor_.SetTextSize(2);
    } else if (stage_ == 10) {
      shot.save("build/ui-large-text-smoke.png");
      editor_.OpenLayout();
    } else if (stage_ == 12) {
      shot.save("build/ui-layout-smoke.png");
      // Click outside the popover using the same input path as the user.
      visage::MouseEvent click;
      click.button_id = visage::kMouseButtonLeft;
      click.position = {4, 4};
      for (auto *child : editor_.children())
        if (child->isVisible() && child->width() == editor_.width())
          child->processMouseDown(click);
      Resize(1440, 900);
      shade_.setVisible(true);
      settings_.setVisible(true);
    } else if (stage_ == 14) {
      shot.save("build/ui-settings-smoke.png");
      settings_.setVisible(false);
      editor_.OpenRouting();
    } else if (stage_ == 16) {
      shot.save("build/ui-routing-smoke.png");
      // Inspect the public preset menu without loading a preset or opening
      // devices.
      for (auto *child : editor_.children())
        if (child->width() == editor_.width() &&
            child->height() == editor_.height())
          child->setVisible(false);
      for (auto *child : editor_.children())
        if (auto *button = dynamic_cast<visage::UiButton *>(child))
          if (child->x() == 16 && child->y() < 48)
            button->onToggle().callback(button, false);
    } else if (stage_ == 18) {
      shot.save("build/ui-presets-smoke.png");
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
