#include "plugin.hpp"
#ifdef DRUMFOUNDRY_UI
#include "editing/routes.hpp"
#endif
#include <cmath>
#include <stdexcept>

namespace drumfoundry::clap_adapter {
namespace {
void Dirty(const clap_host_t *host) {
  const auto *state = host->get_extension
                          ? static_cast<const clap_host_state_t *>(
                                host->get_extension(host, CLAP_EXT_STATE))
                          : nullptr;
  if (state && state->mark_dirty)
    state->mark_dirty(host);
}
} // namespace
void Plugin::SetPreviewStrength(double velocity) {
  if (!std::isfinite(velocity) || velocity < 0 || velocity > 1)
    throw std::invalid_argument(
        "Audition velocity must be between zero and one");
  previewStrength_ = velocity;
  Dirty(host_);
}
#ifdef DRUMFOUNDRY_UI
void Plugin::EditLayout(const Json &positions) {
  PrepareEditorPreset();
  auto next = DesiredDocument();
  editing::ApplyNodePositions(next, positions);
  if (next == document_)
    return;
  document_ = std::move(next);
  Dirty(host_); // No DSP restart/revision for moving a picture of a node.
}
#endif
void Plugin::EditPresentation(const Json &reference, const Json &analysis) {
  PrepareEditorPreset();
  auto next = DesiredDocument();
  next["reference"] = reference;
  next["controls"]["analysis"] = analysis;
  ValidateEnvelope(next);
  // Main-thread metadata only: never reset the sounding voice for a zoom/gain
  // change in the comparison panel. Host state still captures the workbench.
  if (next == document_)
    return;
  document_ = std::move(next);
  documentPreset_ = int(Value(Preset));
  Dirty(host_);
}
void Plugin::PrepareEditorPreset() {
  if (static_cast<int>(Value(Preset)) == documentPreset_)
    return;
  const auto controls = DesiredControls();
  Voice validated(static_cast<float>(sampleRate_), DesiredDocument());
  document_ = validated.Document();
  documentPreset_ = static_cast<int>(controls[0]);
  for (clap_id id = Hardness; id <= Mute; ++id)
    values_[id - Preset].store(controls[id - Preset]);
  values_[ContactSpread - Preset] = controls[ContactSpread - Preset];
  previewStrength_ =
      document_.at("controls").at("event").at("strength").get<double>();
  ++documentRevision_;
}
Json Plugin::EditableDocument() const {
  auto document = DesiredDocument();
  const auto controls = DesiredControls();
  auto &event = document["controls"]["event"];
  event["hardness"] = controls[Hardness - Preset];
  event["implement"] = controls[Implement - Preset];
  event["constraint"] = controls[Mute - Preset];
  event["contactSpread"] = controls[ContactSpread - Preset];
  if (int(controls[0]) == documentPreset_)
    event["strength"] = previewStrength_.load();
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
  values_[ContactSpread - Preset].store(strike.contactSpread);
  previewStrength_ =
      document_.at("controls").at("event").at("strength").get<double>();
  ++documentRevision_;
  if (active)
    RequestRestart();
  if (hostParams_ && hostParams_->rescan)
    hostParams_->rescan(host_, CLAP_PARAM_RESCAN_VALUES);
  Dirty(host_);
}
} // namespace drumfoundry::clap_adapter
