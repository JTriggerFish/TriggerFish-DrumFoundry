#include "membrane_macros.hpp"
#include "tfdsp/percussion/output_eq_parameters.hpp"

#include <array>
#include <cmath>

namespace drumfoundry {
namespace {

using Scale = ParameterScale;

const std::array<ParameterDescriptor, MembraneParameterCount> Descriptors{{
    {"model_level_db", "Model level", "dB", -60.f, 0.f, -6.f},
    {"fundamental_hz", "Fundamental", "Hz", 25.f, 500.f, 105.f,
     Scale::Logarithmic},
    {"decay_seconds", "Body decay", "s", .03f, 8.f, 1.15f, Scale::Logarithmic},
    {"decay_tilt", "Decay tilt", "", -1.f, 1.f, .55f},
    {"inharmonicity", "Membrane character", "", 0.f, 1.f, .35f},
    {"body_brightness", "Body brightness", "", 0.f, 1.f, .55f},
    {"tension_octaves", "Energy pitch lift", "oct", -.25f, .6f, .11f},
    {"tension_decay_seconds", "Tension recovery", "s", .005f, 2.f, .13f,
     Scale::Logarithmic},
    {"contact_direct_level", "Contact to direct", "x", 0.f, 4.f, .245f},
    {"contact_body_level", "Contact to body", "x", 0.f, 4.f, .7f},
    {"contact_duration_seconds", "Contact width", "s", .0002f, .08f, .004f,
     Scale::Logarithmic},
    {"contact_brightness", "Contact brightness", "", 0.f, 1.f, .58f},
    {"fm_direct_level", "FM to direct", "x", 0.f, 3.f, .0144f},
    {"fm_body_level", "FM to body", "x", 0.f, 3.f, .081f},
    {"fm_depth_hz", "FM depth", "Hz", 0.f, 8000.f, 260.f},
    {"fm_decay_seconds", "FM decay", "s", .003f, 1.f, .07f, Scale::Logarithmic},
    {"pitch_drop_octaves", "FM pitch drop", "oct", 0.f, 3.f, .28f},
    {"direct_level", "Direct level", "x", 0.f, 3.f, .9f},
    {"body_level", "Body level", "x", 0.f, 3.f, 3.f},
    {"direct_delay_ms", "Direct delay", "ms", 0.f, 10.f, 0.f},
    {"output_eq_enabled", "Enable final EQ", "", 0.f, 1.f, 1.f, Scale::Boolean},
    {"output_low_cut", "High-pass", "Hz", 5.f, 1000.f, 24.f,
     Scale::Logarithmic},
    {"output_high_cut", "Low-pass", "Hz", 500.f, 22000.f, 18000.f,
     Scale::Logarithmic},
    {"output_colour_frequency", "Colour frequency", "Hz", 40.f, 20000.f, 2800.f,
     Scale::Logarithmic},
    {"output_colour_gain", "Colour gain", "dB", -24.f, 24.f, 0.f},
    {"fm_pitch_decay_seconds", "Pitch fall time", "s", .003f, .5f, .049f,
     Scale::Logarithmic},
    {"contact_noise_level", "Contact noise", "x", 0.f, 4.f, .45f},
    {"contact_noise_decay_seconds", "Noise fade time", "s", .001f, 1.f, .012f,
     Scale::Logarithmic},
    {"output_colour_q", "Peak Q", "", .1f, 20.f, .7f, Scale::Logarithmic},
}};

std::size_t Index(const MembraneParameter parameter) noexcept {
  return static_cast<std::size_t>(parameter);
}

} // namespace

const ParameterDescriptor &
MembraneParameterDescription(const std::size_t index) noexcept {
  return Descriptors[index < Descriptors.size() ? index : 0];
}

MembraneParameterValues DefaultMembraneParameters() noexcept {
  MembraneParameterValues result{};
  for (std::size_t index = 0; index < result.size(); ++index)
    result[index] = Descriptors[index].defaultValue;
  return result;
}

tfdsp::percussion::MembraneDrumParameters
ApplyMembraneParameters(const MembraneParameterValues &values) noexcept {
  using P = MembraneParameter;
  tfdsp::percussion::MembraneDrumControls controls;
  controls.fundamentalHz = values[Index(P::FundamentalHz)];
  controls.decaySeconds = values[Index(P::DecaySeconds)];
  controls.decayTilt = values[Index(P::DecayTilt)];
  controls.inharmonicity = values[Index(P::Inharmonicity)];
  controls.bodyBrightness = values[Index(P::BodyBrightness)];
  controls.tensionOctaves = values[Index(P::TensionOctaves)];
  controls.tensionDecaySeconds = values[Index(P::TensionDecaySeconds)];
  controls.contactDirectLevel = values[Index(P::ContactDirectLevel)];
  controls.contactBodyLevel = values[Index(P::ContactBodyLevel)];
  controls.contactDurationSeconds = values[Index(P::ContactDurationSeconds)];
  controls.contactBrightness = values[Index(P::ContactBrightness)];
  controls.contactNoiseLevel = values[Index(P::ContactNoiseLevel)];
  controls.contactNoiseDecaySeconds =
      values[Index(P::ContactNoiseDecaySeconds)];
  controls.fmDirectLevel = values[Index(P::FmDirectLevel)];
  controls.fmBodyLevel = values[Index(P::FmBodyLevel)];
  controls.fmDepthHz = values[Index(P::FmDepthHz)];
  controls.fmDecaySeconds = values[Index(P::FmDecaySeconds)];
  controls.fmPitchDecaySeconds = values[Index(P::FmPitchDecaySeconds)];
  controls.pitchDropOctaves = values[Index(P::PitchDropOctaves)];
  controls.directLevel = values[Index(P::DirectLevel)];
  controls.bodyLevel = values[Index(P::BodyLevel)];
  controls.directDelaySeconds = .001f * values[Index(P::DirectDelayMs)];
  controls.equalizerMode =
      values[Index(P::OutputEqEnabled)] >= .5f
          ? tfdsp::percussion::ObservationEqualizerMode::Radiation
          : tfdsp::percussion::ObservationEqualizerMode::Bypass;
  controls.lowCutHz = values[Index(P::LowCutHz)];
  controls.highCutHz = values[Index(P::HighCutHz)];
  controls.colourFrequencyHz = values[Index(P::ColourFrequencyHz)];
  controls.colourGainDb = values[Index(P::ColourGainDb)];
  controls.outputGain = std::pow(10.f, values[Index(P::ModelLevelDb)] / 20.f);
  auto result = tfdsp::percussion::DefaultMembraneDrumParameters(controls);
  result.equalizer.radiation = tfdsp::percussion::SimpleOutputEqParameters(
      controls.lowCutHz, controls.colourFrequencyHz, controls.colourGainDb,
      controls.highCutHz, values[Index(P::ColourQ)]);
  return result;
}

} // namespace drumfoundry
