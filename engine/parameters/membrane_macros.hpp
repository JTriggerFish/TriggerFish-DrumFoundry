#pragma once

#include "parameter_descriptor.hpp"
#include "tfdsp/percussion/membrane_drum_parameters.hpp"

#include <array>
#include <cstddef>

namespace drumfoundry {

enum class MembraneParameter : std::size_t {
  ModelLevelDb,
  FundamentalHz,
  DecaySeconds,
  DecayTilt,
  Inharmonicity,
  BodyBrightness,
  TensionOctaves,
  TensionDecaySeconds,
  ContactDirectLevel,
  ContactBodyLevel,
  ContactDurationSeconds,
  ContactBrightness,
  FmDirectLevel,
  FmBodyLevel,
  FmDepthHz,
  FmDecaySeconds,
  PitchDropOctaves,
  DirectLevel,
  BodyLevel,
  DirectDelayMs,
  OutputEqEnabled,
  LowCutHz,
  HighCutHz,
  ColourFrequencyHz,
  ColourGainDb,
  FmPitchDecaySeconds,
  ContactNoiseLevel,
  ContactNoiseDecaySeconds,
  ColourQ,
  Count
};

inline constexpr std::size_t MembraneParameterCount =
    static_cast<std::size_t>(MembraneParameter::Count);
using MembraneParameterValues = std::array<float, MembraneParameterCount>;

const ParameterDescriptor &
MembraneParameterDescription(std::size_t index) noexcept;
MembraneParameterValues DefaultMembraneParameters() noexcept;
tfdsp::percussion::MembraneDrumParameters
ApplyMembraneParameters(const MembraneParameterValues &values) noexcept;

} // namespace drumfoundry
