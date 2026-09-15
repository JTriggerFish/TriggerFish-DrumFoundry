#pragma once
#include "controls.hpp"
namespace drumfoundry::ui {
// One compact row; hover retains the full device status and any error text.
class StatusLine : public visage::Frame, public HelpText {
public:
  void Set(std::string meter, std::string status, std::string error,
           bool limiting) {
    if (meter_ == meter && status_ == status && error_ == error &&
        limiting_ == limiting)
      return;
    meter_ = std::move(meter);
    status_ = std::move(status);
    error_ = std::move(error);
    limiting_ = limiting;
    help = meter_ + "\n" + status_ +
           (error_.empty() ? "" : "\nError: " + error_);
    redraw();
  }
  void draw(visage::Canvas &c) override {
    const float meterWidth =
        std::min(width() * .45f, 350 * paletteValue(TextScale));
    Label(c, ElideText(FrameFont(*this), meter_, meterWidth), 0, 0,
          meterWidth, height(),
          limiting_ ? colours::Warning : colours::Muted);
    Label(c,
          ElideText(FrameFont(*this),
                    error_.empty() ? status_ : "Error: " + error_,
                    width() - meterWidth - 16),
          meterWidth + 16, 0, width() - meterWidth - 16, height(),
          error_.empty() ? colours::Muted : colours::Error);
  }

private:
  std::string meter_, status_, error_;
  bool limiting_{};
};
} // namespace drumfoundry::ui
