#include "plugin.hpp"
#include "builtin_presets.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace drumfoundry::clap_adapter {
static_assert(std::atomic<double>::is_always_lock_free,
              "Host controls require lock-free scalar publication");
bool Plugin::Init() {
  for (std::size_t i = 0; i < Controls.size(); ++i)
    values_[i].store(Controls[i].initial);
  Voice initial(48000, ParseJson(PresetJson[0]));
  document_ = initial.Document();
  const auto &strike = initial.Event();
  values_[Hardness - Preset] = strike.hardness;
  values_[Implement - Preset] = strike.implement;
  values_[Location - Preset] = strike.location;
  values_[Mute - Preset] = strike.constraint;
  values_[ContactSpread - Preset] = strike.contactSpread;
  previewStrength_ =
      document_.at("controls").at("event").at("strength").get<double>();
  if (host_->get_extension) {
    hostParams_ = static_cast<const clap_host_params_t *>(
        host_->get_extension(host_, CLAP_EXT_PARAMS));
    hostLatency_ = static_cast<const clap_host_latency_t *>(
        host_->get_extension(host_, CLAP_EXT_LATENCY));
  }
  return true;
}
Json Plugin::DesiredDocument() const {
  const int index = static_cast<int>(Value(Preset));
  return index == documentPreset_ ? document_ : ParseJson(PresetJson.at(index));
}
std::array<double, ParameterCount> Plugin::DesiredControls() const {
  std::array<double, ParameterCount> result{};
  for (clap_id id = Preset; id < ParameterEnd; ++id)
    result[id - Preset] = Value(id);
  if (static_cast<int>(result[0]) != documentPreset_) {
    // Selecting an instrument restores its saved gesture, not a generic stick
    // that would silently turn the mallet gong into a different sound.
    const auto document = DesiredDocument();
    const auto strike =
        ReadStrike(document.at("controls").at("event"), result[0] == 0);
    result[Hardness - Preset] = strike.hardness;
    result[Implement - Preset] = strike.implement;
    result[Location - Preset] = strike.location;
    result[Mute - Preset] = strike.constraint;
    result[ContactSpread - Preset] = strike.contactSpread;
  }
  return result;
}
bool Plugin::Activate(double rate, uint32_t minimum, uint32_t maximum) {
  if (active || minimum < 1 || maximum < minimum || maximum > 1048576)
    return false;
  auto next =
      std::make_unique<Voice>(static_cast<float>(rate), DesiredDocument());
  const auto controls = DesiredControls();
  output::Limiter protection;
  protection.Prepare(rate, 2, Value(Protection) >= .5);
  auto stored = next->Document();
  if (int(Value(Preset)) != documentPreset_)
    previewStrength_ =
        stored.at("controls").at("event").at("strength").get<double>();
  document_ = std::move(stored);
  documentPreset_ = static_cast<int>(Value(Preset));
  voice_ = std::move(next);
  limiter_ = std::move(protection);
  sampleRate_ = rate;
#ifdef DRUMFOUNDRY_UI
  audition_.Stop();
  auditionRate_ = unsigned(std::lround(rate));
  outputTap_.Reset(unsigned(std::lround(rate)));
#endif
  maximumFrames_ = maximum;
  for (std::size_t i = 0; i < controls.size(); ++i)
    values_[i].store(controls[i]);
  for (std::size_t i = 0; i < Controls.size(); ++i)
    audioValues_[i] = values_[i].load();
  masterStep_ = -std::expm1(-1 / (.005 * rate));
  masterGain_ = masterTarget_ = std::pow(10., Value(Master) / 20);
  reductionHold_ = 0;
  values_[Reduction - Preset].store(0);
  const auto oldLatency = latency_;
  latency_ = static_cast<uint32_t>(limiter_.Status().latencySamples);
  values_[Latency - Preset].store(1000. * latency_ / rate);
  active = true;
  restartQueued_.store(false);
  if (oldLatency != latency_ && hostLatency_ && hostLatency_->changed)
    hostLatency_->changed(host_);
  if (hostParams_ && hostParams_->rescan)
    hostParams_->rescan(host_, CLAP_PARAM_RESCAN_VALUES);
  return true;
}
void Plugin::Deactivate() noexcept {
#ifdef DRUMFOUNDRY_UI
  audition_.Stop();
  auditionRate_ = 0;
  outputTap_.Reset(0);
#endif
  processing = active = false;
  voice_.reset(); // Only after the host has stopped calling Process.
  // These are active-output readouts, not saved synthesis parameters.
  latency_ = 0;
  values_[Latency - Preset].store(0);
  values_[Reduction - Preset].store(0);
}
void Plugin::Reset() noexcept {
#ifdef DRUMFOUNDRY_UI
  audition_.Stop();
#endif
  if (voice_)
    voice_->Reset();
  limiter_.Reset();
  masterGain_ = masterTarget_;
  reductionHold_ = 0;
  values_[Reduction - Preset].store(0);
}
void Plugin::RequestRestart() noexcept {
  if (!restartQueued_.exchange(true) && host_->request_restart)
    host_->request_restart(host_);
}
void Plugin::OnMainThread() noexcept {
  // Reserved for the editor's main-thread work; no background worker owns
  // voices.
}
const void *Plugin::Extension(const char *id) const noexcept {
  if (!id)
    return nullptr;
#ifdef DRUMFOUNDRY_UI
  if (!std::strcmp(id, CLAP_EXT_GUI))
    return &GuiExtension;
#if defined(__linux__)
  if (!std::strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT))
    return &FdExtension;
#endif
#endif
  if (!std::strcmp(id, CLAP_EXT_PARAMS))
    return &ParamsExtension;
  if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
    return &AudioExtension;
  if (!std::strcmp(id, CLAP_EXT_NOTE_PORTS))
    return &NoteExtension;
  if (!std::strcmp(id, CLAP_EXT_LATENCY))
    return &LatencyExtension;
  if (!std::strcmp(id, CLAP_EXT_STATE))
    return &StateExtension;
  return nullptr;
}
} // namespace drumfoundry::clap_adapter
