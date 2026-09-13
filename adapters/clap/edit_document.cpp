#include "plugin.hpp"

namespace drumfoundry::clap_adapter {
void Plugin::PrepareEditorPreset() {
  if (static_cast<int>(Value(Preset)) == documentPreset_)
    return;
  const auto controls = DesiredControls();
  Voice validated(static_cast<float>(sampleRate_), DesiredDocument());
  document_ = validated.Document();
  documentPreset_ = static_cast<int>(controls[0]);
  for (clap_id id = Hardness; id <= Mute; ++id)
    values_[id - Preset].store(controls[id - Preset]);
  ++documentRevision_;
}
Json Plugin::EditableDocument() const {
  auto document = DesiredDocument();
  const auto controls = DesiredControls();
  auto &event = document["controls"]["event"];
  event["hardness"] = controls[Hardness - Preset];
  event["implement"] = controls[Implement - Preset];
  event["constraint"] = controls[Mute - Preset];
  if (controls[0] != 0)
    event["location"] = controls[Location - Preset];
  return document;
}
void Plugin::EditDocument(Json document) {
  Voice validated(static_cast<float>(sampleRate_), std::move(document));
  auto next = validated.Document();
  const auto recipe = Instrument(next).at("recipe").get<std::string>();
  const int preset = recipe == "drum.kick.v1"    ? 0
                     : recipe == "drum.snare.v1" ? 1
                     : Value(Preset) >= 2 ? static_cast<int>(Value(Preset))
                                          : 3;
  const auto strike = validated.Event();
  document_ = std::move(next);
  documentPreset_ = preset;
  values_[Preset - Preset].store(preset);
  values_[Hardness - Preset].store(strike.hardness);
  values_[Implement - Preset].store(strike.implement);
  values_[Location - Preset].store(strike.location);
  values_[Mute - Preset].store(strike.constraint);
  ++documentRevision_;
  if (active)
    RequestRestart();
  if (hostParams_ && hostParams_->rescan)
    hostParams_->rescan(host_, CLAP_PARAM_RESCAN_VALUES);
  const auto *state = host_->get_extension
                          ? static_cast<const clap_host_state_t *>(
                                host_->get_extension(host_, CLAP_EXT_STATE))
                          : nullptr;
  if (state && state->mark_dirty)
    state->mark_dirty(host_);
}
} // namespace drumfoundry::clap_adapter
