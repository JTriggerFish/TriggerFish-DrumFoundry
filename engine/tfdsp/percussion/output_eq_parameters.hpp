#pragma once
#include "radiation_filter.hpp"

namespace tfdsp::percussion {
// One final presentation EQ shared by instrument recipes and their editor.
// Fixed, broad bandwidths; no extra resonant or per-source controls.
inline RadiationFilterParameters
SimpleOutputEqParameters(float lowCut, float colourFrequency, float colourGain,
                         float highCut) noexcept {
  RadiationFilterParameters p;
  p.lowCutHz = std::clamp(lowCut, 5.f, 1000.f);
  p.highCutHz = std::clamp(highCut, 500.f, 22000.f);
  p.colourFrequencyHz = std::clamp(colourFrequency, 40.f, 20000.f);
  p.colourGainDb = std::clamp(colourGain, -24.f, 24.f);
  p.lowCutQ = p.highCutQ = .70710678f;
  p.colourQ = .7f;
  return p;
}
} // namespace tfdsp::percussion
