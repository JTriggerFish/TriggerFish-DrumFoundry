#include "voice.hpp"
#include "live_controls.hpp"
#include <cmath>
#include <stdexcept>

namespace drumfoundry {
Voice::Voice(float sampleRate, Json document) : sampleRate_(sampleRate) {
  if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 384000)
    throw std::invalid_argument("Unsupported sample rate");
  Configure(std::move(document));
}
void Voice::Configure(Json document) {
  ValidateEnvelope(document);
  auto &patch = Instrument(document);
  UpgradeOutputEq(patch);
  const auto recipe = ParseRecipe(patch.at("recipe").get<std::string>());
  Strike event;
  if (document.contains("instrument"))
    event = ReadStrike(document.at("controls").at("event"),
                       recipe == detail::Recipe::Kick);
  auto next = std::make_unique<detail::Session>();
  detail::Initialize(*next, recipe, sampleRate_);
  ApplyPatch(*next, patch);
  detail::Prepare(*next);
  session_.swap(next);
  liveIndices_.fill(false);
  const auto recipeName = patch.at("recipe").get<std::string>();
  for (std::size_t i = 0; i < detail::ParameterCount(*session_); ++i)
    liveIndices_[i] =
        IsLiveParameter(recipeName, detail::Description(*session_, i)->key);
  liveDirty_ = decayDirty_ = false;
  document_ = std::move(document);
  event_ = event;
}
void Voice::Reset() noexcept {
  switch (session_->recipe) {
  case detail::Recipe::MetallicPlate:
    session_->cymbal.Reset();
    break;
  case detail::Recipe::Kick:
    session_->kick.Reset();
    break;
  case detail::Recipe::MembraneDrum:
    session_->membrane.Reset();
    break;
  case detail::Recipe::SnareDrum:
    session_->snare.Reset();
    break;
  default:
    break;
  }
}
void Voice::Trigger(const Strike &e) noexcept {
  FlushParameters();
  auto &s = *session_;
  const tfdsp::percussion::MembraneDrumHit hit{
      e.strength, e.location, e.hardness, e.implement, e.contactSpread, e.seed};
  switch (s.recipe) {
  case detail::Recipe::MetallicPlate:
    s.cymbal.SetMute(e.constraint);
    s.cymbal.Trigger({e.strength, e.location, e.hardness, e.seed, e.implement,
                      e.contactSpread});
    break;
  case detail::Recipe::Kick: {
    auto fixed = hit;
    fixed.location = 0.f;
    s.kick.Trigger(fixed);
    break;
  }
  case detail::Recipe::MembraneDrum:
    s.membrane.Trigger(hit);
    break;
  case detail::Recipe::SnareDrum:
    s.snare.Trigger(hit);
    break;
  default:
    break;
  }
}
void Voice::SetMute(float amount) noexcept {
  if (session_->recipe == detail::Recipe::MetallicPlate)
    session_->cymbal.SetMute(amount);
}
void Voice::Process(float *output, std::size_t frames) noexcept {
  if (frames)
    FlushParameters();
  for (std::size_t i = 0; i < frames; ++i)
    output[i] = detail::Process(*session_);
}
} // namespace drumfoundry
