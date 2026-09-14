#pragma once

#include "kick_mode_macros.hpp"
#include "parameter_descriptor.hpp"
#include "tfdsp/percussion/kick_voice_parameters.hpp"
#include <array>
#include <cstddef>

namespace drumfoundry {
enum class KickParameter : std::size_t {
  ModelLevelDb,
  ContactLevel,
  ContactObservation,
  ContactBodyDrive,
  ContactWidth,
  ContactColour,
  ContactNoise,
  ContactNoiseDecay,
  ThumpLevel,
  ThumpPitch,
  ThumpDrop,
  ThumpFall,
  ThumpDecay,
  ThumpHold,
  ThumpDecayShape,
  ResonanceLevel,
  ResonanceDecay,
  ResonanceDecayTilt,
  TensionOctaves,
  TensionRecovery,
  OutputEqEnabled,
  LowCutHz,
  HighCutHz,
  ColourFrequency,
  ColourGain,
  Count
};
inline constexpr std::size_t KickParameterCount =
    static_cast<std::size_t>(KickParameter::Count) + KickModeParameterCount;
using KickParameterValues = std::array<float, KickParameterCount>;
const ParameterDescriptor &KickParameterDescription(std::size_t index) noexcept;
KickParameterValues DefaultKickParameters() noexcept;
tfdsp::percussion::MembraneDrumParameters
ApplyKickParameters(const KickParameterValues &values) noexcept;
} // namespace drumfoundry
