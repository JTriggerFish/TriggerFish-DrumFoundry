#include "decay_hold_panel.hpp"
#include "preview_tracker.hpp"
#include <iomanip>
#include <sstream>
namespace drumfoundry::ui {
DecayHoldPanel::DecayHoldPanel() {
  addChild(&enabled_);
  addChild(&cancel_, false);
  enabled_.help =
      "After changing bloom, try to keep the old 1–6 second tail while "
      "preserving your new attack. Adjusts visible T60 knots and, unless you "
      "changed it, concentration dependence. No gain matching or extra "
      "envelope. Large changes may not be compensable.";
  enabled_.onToggle() = [this](auto *, bool) {
    enabledValue_ = !enabledValue_;
    enabled_.setText(enabledValue_ ? "Hold decay ON" : "Hold decay OFF");
    Cancel();
    status_ = enabledValue_ ? "Ready for a bloom edit."
                            : "Decay controls stay fixed.";
    redraw();
  };
  cancel_.onToggle() = [this](auto *, bool) { Cancel(); };
}
editing::Json DecayHoldPanel::Sound(const editing::Json &d) {
  return SoundIdentity(d);
}
void DecayHoldPanel::Cancel() {
  worker_.Cancel();
  if (waiting_ || pending_)
    status_ = "Cancelled; your edit is unchanged.";
  pending_ = waiting_ = false;
  cancel_.setVisible(false);
  redraw();
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
  cancel_.setVisible(true);
  status_ = "Checking bloom edit…";
  redraw();
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
  if (!waiting_)
    return;
  if (auto completion = worker_.Take()) {
    waiting_ = false;
    cancel_.setVisible(false);
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
  redraw();
}
void DecayHoldPanel::resized() {
  enabled_.setBounds(0, 0, width() - 84, 28);
  cancel_.setBounds(width() - 78, 0, 74, 28);
}
void DecayHoldPanel::draw(visage::Canvas &c) {
  Label(c, status_, 0, 32, width() - 4, 54);
}
} // namespace drumfoundry::ui
