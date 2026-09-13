#include "builtin_presets.hpp"
#include "plugin.hpp"
#ifdef DRUMFOUNDRY_UI
#include "editing/routes.hpp"
#include "ui/analysis_validation.hpp"
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
  if (int(Value(Preset)) != documentPreset_)
    throw std::runtime_error(
        "Instrument changed while editing layout; please retry");
  auto next = document_;
  editing::ApplyNodePositions(next, positions);
  if (next == document_)
    return;
  document_ = std::move(next);
  Dirty(host_); // No DSP restart/revision for moving a picture of a node.
}
#endif
void Plugin::EditPresentation(const Json &reference, const Json &analysis) {
  PrepareEditorPreset();
  if (int(Value(Preset)) != documentPreset_)
    throw std::runtime_error(
        "Instrument changed while editing presentation; please retry");
  auto next = document_;
  next["reference"] = reference;
  next["controls"]["analysis"] = analysis;
  ValidateEnvelope(next);
  // Main-thread metadata only: never reset the sounding voice for a zoom/gain
  // change in the comparison panel. Host state still captures the workbench.
  if (next == document_)
    return;
  document_ = std::move(next);
  Dirty(host_);
}
void Plugin::PrepareEditorPreset() {
  if (static_cast<int>(Value(Preset)) == documentPreset_)
    return;
  const auto desired = CaptureDesired();
  const auto &controls = desired.controls;
  Voice validated(static_cast<float>(sampleRate_), desired.document);
  if (Value(Preset) != controls[0])
    return; // A newer selection is pending.
  document_ = validated.Document();
  documentPreset_ = static_cast<int>(controls[0]);
  for (clap_id id = Hardness; id <= Mute; ++id)
    values_[id - Preset].store(controls[id - Preset]);
  values_[ContactSpread - Preset] = controls[ContactSpread - Preset];
  previewStrength_ =
      document_.at("controls").at("event").at("strength").get<double>();
  ++documentRevision_;
}
Json Plugin::EditableDocument() const { return CaptureDesired().document; }
void Plugin::EditDocument(Json document) {
  document = WithFitEnvelope(std::move(document));
#ifdef DRUMFOUNDRY_UI
  ui::ValidateAnalysisDocument(document);
#endif
  Voice validated(static_cast<float>(sampleRate_), std::move(document));
  auto next = validated.Document();
  const auto recipe = Instrument(next).at("recipe").get<std::string>();
  const int selected = static_cast<int>(Value(Preset));
  const int preset = recipe == "drum.kick.v1"    ? 0
                     : recipe == "drum.snare.v1" ? 1
                     : selected >= 2             ? selected
                                                 : 3;
  PublishDocument(validated, preset);
}
void Plugin::SelectFactory(unsigned index) {
  Voice validated(static_cast<float>(sampleRate_),
                  ParseJson(PresetJson.at(index)));
  PublishDocument(validated, int(index));
}
void Plugin::SelectCalibration(unsigned index) {
  Voice validated(static_cast<float>(sampleRate_),
                  ParseJson(CalibrationJson.at(index)));
  PublishDocument(validated, int(index));
}
void Plugin::PublishDocument(const Voice &validated, int preset) {
  const auto strike = validated.Event();
  auto next = validated.Document();
  const double strength = next.at("controls").at("event").at("strength");
  document_ = std::move(next);
  documentPreset_ = preset;
  values_[Preset - Preset].store(preset);
  values_[Hardness - Preset].store(strike.hardness);
  values_[Implement - Preset].store(strike.implement);
  values_[Location - Preset].store(strike.location);
  values_[Mute - Preset].store(strike.constraint);
  values_[ContactSpread - Preset].store(strike.contactSpread);
  previewStrength_ = strength;
  ++documentRevision_;
  if (active)
    RequestRestart();
  if (hostParams_ && hostParams_->rescan)
    hostParams_->rescan(host_, CLAP_PARAM_RESCAN_VALUES);
  Dirty(host_);
}
} // namespace drumfoundry::clap_adapter
