#include "kick_macros.hpp"
#include "tfdsp/percussion/output_eq_parameters.hpp"
#include <cmath>

namespace drumfoundry {
namespace {
using Scale = ParameterScale;
const std::array<ParameterDescriptor,
                 static_cast<std::size_t>(KickParameter::Count)>
    Descriptors{{
        {"model_level_db", "Model level", "dB", -60.f, 0.f, -12.f},
        {"contact_level", "Contact level", "x", 0.f, 4.f, 0.4f},
        {"contact_observation", "Contact observation", "mode", 0.f, 1.f, 0.f,
         Scale::Choice},
        {"contact_body_drive", "Body drive", "mode", 0.f, 1.f, 0.f,
         Scale::Choice},
        {"contact_width_seconds", "Contact width", "s", 0.0002f, 0.08f, 0.011f,
         Scale::Logarithmic},
        {"contact_colour", "Contact colour", "", 0.f, 1.f, 0.33f},
        {"contact_noise_level", "Noise amount", "x", 0.f, 4.f, 0.57f},
        {"contact_noise_decay_seconds", "Noise decay (T60)", "s", 0.00075f,
         0.75f, 0.205f, Scale::Logarithmic},
        {"thump_level", "Thump level", "x", 0.f, 4.f, 1.77f},
        {"thump_pitch_hz", "Thump pitch", "Hz", 20.f, 180.f, 28.f,
         Scale::Logarithmic},
        {"thump_pitch_drop_octaves", "Pitch drop", "oct", 0.f, 4.f, 1.47f},
        {"thump_pitch_fall_seconds", "Pitch fall time", "s", 0.003f, 0.5f,
         0.059f, Scale::Logarithmic},
        {"thump_decay_seconds", "Thump decay (T60)", "s", 0.005f, 3.f, 0.306f,
         Scale::Logarithmic},
        {"thump_hold_seconds", "Thump hold", "s", 0.f, .08f, 0.f},
        {"thump_decay_shape", "Thump decay shape", "", 0.f, 1.f, 0.f},
        {"resonance_level", "Resonance prominence", "x", 0.f, 12.f, 4.72f},
        {"resonance_decay_seconds", "Resonance T60 at 100 Hz", "s", 0.03f, 8.f,
         0.6f, Scale::Logarithmic},
        {"resonance_decay_tilt", "Damping slope", "oct/oct", -1.f, 1.f, .5f},
        {"tension_octaves", "Energy pitch lift", "oct", 0.f, 0.6f, 0.056f},
        {"tension_recovery_seconds", "Tension recovery", "s", 0.005f, 2.f,
         0.012f, Scale::Logarithmic},
        {"output_eq_enabled", "Enable final EQ", "", 0.f, 1.f, 0.f,
         Scale::Boolean},
        {"output_low_cut", "High-pass", "Hz", 5.f, 1000.f, 43.f,
         Scale::Logarithmic},
        {"output_high_cut", "Low-pass", "Hz", 500.f, 22000.f, 1300.f,
         Scale::Logarithmic},
        {"output_colour_frequency", "Colour frequency", "Hz", 40.f, 20000.f,
         1040.f, Scale::Logarithmic},
        {"output_colour_gain", "Colour gain", "dB", -24.f, 24.f, 0.f},
        {"output_colour_q", "Peak Q", "", .1f, 20.f, .7f, Scale::Logarithmic},
    }};
} // namespace

const ParameterDescriptor &
KickParameterDescription(std::size_t index) noexcept {
  return index < Descriptors.size()
             ? Descriptors[index]
             : KickModeDescription(index - Descriptors.size());
}
KickParameterValues DefaultKickParameters() noexcept {
  KickParameterValues result{};
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i] = KickParameterDescription(i).defaultValue;
  return result;
}

tfdsp::percussion::MembraneDrumParameters
ApplyKickParameters(const KickParameterValues &values) noexcept {
  using P = KickParameter;
  const auto get = [&](P p) { return values[static_cast<std::size_t>(p)]; };
  tfdsp::percussion::KickVoiceControls controls;
  controls.contactLevel = get(P::ContactLevel);
  controls.contactNoiseObservationOnly = get(P::ContactObservation) >= .5f;
  controls.contactPulseDriveOnly = get(P::ContactBodyDrive) >= .5f;
  controls.contactWidthSeconds = get(P::ContactWidth);
  controls.contactColour = get(P::ContactColour);
  controls.contactNoiseLevel = get(P::ContactNoise);
  controls.contactNoiseDecaySeconds = get(P::ContactNoiseDecay);
  controls.thumpLevel = get(P::ThumpLevel);
  controls.thumpPitchHz = get(P::ThumpPitch);
  controls.thumpPitchDropOctaves = get(P::ThumpDrop);
  controls.thumpPitchFallSeconds = get(P::ThumpFall);
  controls.thumpDecaySeconds = get(P::ThumpDecay);
  controls.thumpHoldSeconds = get(P::ThumpHold);
  controls.thumpDecayShape = get(P::ThumpDecayShape);
  controls.resonanceLevel = get(P::ResonanceLevel);
  controls.resonanceDecaySeconds = get(P::ResonanceDecay);
  controls.resonanceDecayTilt = get(P::ResonanceDecayTilt);
  controls.tensionOctaves = get(P::TensionOctaves);
  controls.tensionRecoverySeconds = get(P::TensionRecovery);
  controls.outputGain = std::pow(10.f, get(P::ModelLevelDb) / 20.f);
  for (std::size_t i = 0; i < controls.modes.size(); ++i) {
    const auto offset = static_cast<std::size_t>(P::Count) + 2 * i;
    controls.modes[i] = {values[offset], values[offset + 1]};
  }
  auto result = tfdsp::percussion::DefaultKickVoiceParameters(controls);
  auto &eq = result.equalizer;
  eq.mode = get(P::OutputEqEnabled) >= .5f
                ? tfdsp::percussion::ObservationEqualizerMode::Radiation
                : tfdsp::percussion::ObservationEqualizerMode::Bypass;
  eq.radiation = tfdsp::percussion::SimpleOutputEqParameters(
      get(P::LowCutHz), get(P::ColourFrequency), get(P::ColourGain),
      get(P::HighCutHz), get(P::ColourQ));
  return result;
}
} // namespace drumfoundry
