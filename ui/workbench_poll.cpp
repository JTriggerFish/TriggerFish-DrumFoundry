#include "workbench.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
namespace drumfoundry::ui {
void Workbench::PollPerformance() {
  preset_.setText("Presets ▾");
  audioRunning_ = !bridge_.audioRunning || bridge_.audioRunning();
  settings_.setActionButton(!audioRunning_);
  master_.Set(bridge_.value(105));
  hardness_.Set(bridge_.value(101));
  spread_.Set(bridge_.value(109));
  if (bridge_.velocity)
    velocity_.Set(bridge_.velocity());
  const double implement = bridge_.value(102);
  for (unsigned i = 0; i < implements_.size(); ++i)
    implements_[i].setActionButton(std::abs(implement - i * .5) < .01);
  hardness_.SetLabel(implement < .25   ? "Bristle stiffness"
                     : implement < .75 ? "Mallet firmness"
                                       : "Tip hardness");
  const bool kick = document_.Recipe() == "drum.kick.v1";
  strike_.SetKick(kick);
  strike_.SetMembrane(document_.Recipe().find("drum.") == 0 && !kick);
  location_.setVisible(!kick);
  mute_.setVisible(document_.Recipe() == "metal.cymbal.v1");
  location_.Set(bridge_.value(103));
  mute_.Set(bridge_.value(104));
  limiter_.setText(bridge_.value(106) >= .5 ? "Limiter ON" : "UNPROTECTED");
  reduction_ = bridge_.value(107);
  latency_ = bridge_.value(108);
}
void Workbench::PollPreview() {
  if (!bridge_.document || document_.JsonValue().is_null())
    return;
  auto sound = document_.JsonValue();
  sound["controls"]["event"] = bridge_.document().at("controls").at("event");
  if (holdDecay_.NeedsPoll())
    holdDecay_.Poll(sound);
  if (reloadDocument_)
    return;
  if (preview_.Advance(sound))
    analysis_.UpdateModel(sound);
}
void Workbench::Poll() {
  help_.Poll();
  try {
    liveSpectrum_.Poll(bridge_);
    excitation_.RefreshSpectrum();
    if (bridge_.service)
      bridge_.service();
    if (bridge_.document &&
        (reloadDocument_ || documentPreset_ != int(bridge_.value(100)) ||
         (bridge_.revision && documentRevision_ != bridge_.revision())))
      RefreshDocument();
    if (bridge_.sampleRate)
      analysis_.SetAuditionRate(bridge_.sampleRate());
    if (analysis_.PollLive(bridge_) && !document_.JsonValue().is_null()) {
      auto sound = document_.JsonValue();
      sound["controls"]["event"] =
          bridge_.document().at("controls").at("event");
      preview_.Reset(
          sound); // A strike updates live analysis, not an offline replay.
    }
    analysis_.Poll();
    PollPreview();
    PollPerformance();
    if (bridge_.status)
      status_ = bridge_.status();
    redraw();
  } catch (const std::exception &e) {
    Error(e.what());
  }
}
} // namespace drumfoundry::ui
