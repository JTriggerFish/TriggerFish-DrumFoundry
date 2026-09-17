#include "decay_hold_panel.hpp"
#include "preview_tracker.hpp"
#include <iomanip>
#include <sstream>
namespace drumfoundry::ui {
DecayHoldPanel::DecayHoldPanel() {
  setName("bloom-actions");
  addChild(&timing_);
  addChild(&enabled_);
  timing_.setName("bloom-timing");
  enabled_.setName("hold-decay");
  timing_.help = "Move several bloom controls together towards an earlier or "
                 "later bloom. Opens the timing control and its explanation.";
  timing_.onToggle() = [this](auto *, bool) {
    if (timing) timing();
  };
  enabled_.onToggle() = [this](auto *, bool) {
    if (NeedsPoll()) {
      Cancel(); // The busy button reads Cancel; the enabled preference stays on.
      return;
    }
    enabledValue_ = !enabledValue_;
    Cancel();
    status_ = enabledValue_ ? "Ready for a bloom edit."
                            : "Decay controls stay fixed.";
    RefreshControls();
  };
  RefreshControls();
}
editing::Json DecayHoldPanel::Sound(const editing::Json &d) {
  return SoundIdentity(d);
}
void DecayHoldPanel::Cancel() {
  worker_.Cancel();
  if (waiting_ || pending_)
    status_ = "Cancelled; your edit is unchanged.";
  pending_ = waiting_ = false;
  RefreshControls();
}
void DecayHoldPanel::Edited(const editing::Json &before,
                            const editing::Json &after, unsigned rate) {
  Cancel();
  if (!enabledValue_ ||
      after.at("instrument").at("recipe") != "metal.cymbal.v1")
    return;
  if (Sound(before) == Sound(after))
    return;
  baseline_ = before;
  edited_ = after;
  expected_ = Sound(after);
  rate_ = rate;
  started_ = std::chrono::steady_clock::now();
  pending_ = true;
  status_ = "Checking bloom edit…";
  RefreshControls();
}
void DecayHoldPanel::Poll(const editing::Json &current) {
  if ((waiting_ || pending_) && Sound(current) != expected_) {
    Cancel();
    return;
  }
  const double elapsed =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - started_)
          .count();
  if (pending_ && !worker_.Busy() && elapsed >= .35) {
    pending_ = false;
    waiting_ = worker_.Start(baseline_, edited_, rate_);
  }
  if (!waiting_) {
    RefreshControls();
    return;
  }
  if (auto completion = worker_.Take()) {
    waiting_ = false;
    if (!completion->error.empty()) {
      status_ = "Hold failed; your edit is unchanged.";
      if (error)
        error(completion->error);
    } else {
      const auto &result = completion->result;
      std::ostringstream text;
      if (!result.reason.empty())
        text << result.reason << " · " << std::fixed << std::setprecision(2)
             << result.before << " → " << result.after << " dB tail change"
             << (result.accepted ? " · visible controls updated"
                                 : " · your edit kept");
      status_ = text.str();
      if (result.accepted && apply)
        apply(result);
    }
  } else {
    std::ostringstream text;
    text << "Holding decay… " << std::fixed << std::setprecision(1) << elapsed
         << " s · " << worker_.Evaluations() << " renders";
    status_ = text.str();
  }
  RefreshControls();
}
void DecayHoldPanel::resized() {
  const float half = std::max(1.f, (width() - 8) / 2);
  timing_.setBounds(0, 0, half, height());
  enabled_.setBounds(half + 8, 0, half, height());
  RefreshControls();
}
void DecayHoldPanel::RefreshControls() {
  std::ostringstream label;
  if (NeedsPoll()) {
    const double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started_).count();
    label << "Cancel · " << std::fixed << std::setprecision(1) << elapsed << " s";
  } else label << (enabledValue_ ? "Hold decay ON" : "Hold decay OFF");
  const auto fits = [this](const std::string &text) {
    return FrameFont(*this).stringWidth(visage::String(text).toUtf32()) <=
           enabled_.width() - 12;
  };
  timing_.setText(fits("Bloom timing…") ? "Bloom timing…" : "Timing…");
  std::string caption = label.str();
  if (!fits(caption)) {
    if (NeedsPoll()) {
      const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - started_).count();
      caption = "Cancel " + std::to_string(seconds) + "s";
    } else caption = enabledValue_ ? "Hold ON" : "Hold OFF";
  }
  enabled_.setText(caption);
  enabled_.setActionButton(enabledValue_);
  enabled_.help = "Keep a similar tail after changing bloom by adjusting the "
                  "visible decay controls. While working, click to cancel. "
                  "Large changes may not be compensable.\n" + status_;
  redraw();
}
} // namespace drumfoundry::ui
