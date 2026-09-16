#include "live_controls.hpp"
#include "parameters/validation.hpp"
#include "voice.hpp"
#include <cmath>

namespace drumfoundry {
bool Voice::StageParameter(std::size_t i, float value) noexcept {
  if (i >= liveIndices_.size() || !liveIndices_[i] || !std::isfinite(value))
    return false;
  auto &s = *session_;
  const auto &d = *detail::Description(s, i);
  if (!ValidParameterValue(d, value))
    return false;
  if (s.recipe == detail::Recipe::MetallicPlate) {
    // Preserve the same endpoint constraint as the curve editor/JSON contract.
    auto values = s.crashValues;
    values[i] = value;
    if (!ValidLiveDecay(values))
      return false;
    s.crashValues = values;
    decayDirty_ |= d.key.rfind("body_decay_", 0) == 0 &&
                   d.key != "body_decay_friction";
  } else if (s.recipe == detail::Recipe::Kick)
    s.kickValues[i] = value;
  else if (s.recipe == detail::Recipe::MembraneDrum)
    s.membraneValues[i] = value;
  else if (s.recipe == detail::Recipe::SnareDrum)
    s.snareValues[i] = value;
  liveDirty_ = true;
  return true;
}

void Voice::FlushParameters() noexcept {
  if (!liveDirty_)
    return;
  auto &s = *session_;
  using namespace tfdsp::percussion;
  if (s.recipe == detail::Recipe::MetallicPlate) {
    const auto fit = ApplyCrashMacros(s.crashBase, s.crashValues);
    s.cymbal.SetLiveControls(fit);
    if (decayDirty_)
      s.cymbal.SetLiveDecayCurve(fit);
  } else {
    auto snare = s.recipe == detail::Recipe::SnareDrum
                     ? ApplySnareParameters(s.snareValues)
                     : SnareDrumParameters{};
    const auto membrane = s.recipe == detail::Recipe::Kick
                              ? ApplyKickParameters(s.kickValues)
                          : s.recipe == detail::Recipe::MembraneDrum
                              ? ApplyMembraneParameters(s.membraneValues)
                              : snare.membrane;
    std::array<float, MembraneModeCount> radii{};
    for (std::size_t i = 0; i < radii.size(); ++i)
      radii[i] =
          std::exp(std::log(.001f) /
                   (std::clamp(membrane.membrane[i].decaySeconds, .002f, 30.f) *
                    sampleRate_));
    if (s.recipe == detail::Recipe::SnareDrum) {
      s.snare.SetLiveControls(snare);
      s.snare.SetLiveDecay(radii);
    } else {
      auto &body = s.recipe == detail::Recipe::Kick ? s.kick : s.membrane;
      body.SetLiveControls(membrane);
      body.SetLiveDecay(radii);
    }
  }
  liveDirty_ = decayDirty_ = false;
}
} // namespace drumfoundry
