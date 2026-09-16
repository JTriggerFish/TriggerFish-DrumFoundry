#pragma once
#include "radiation_filter.hpp"

namespace tfdsp::percussion {
// One final presentation EQ shared by instrument recipes and their editor.
// Butterworth cuts and one variable-width bell; no per-source EQ controls.
inline RadiationFilterParameters
SimpleOutputEqParameters(float lowCut, float colourFrequency, float colourGain,
                         float highCut, float colourQ = .7f) noexcept {
  RadiationFilterParameters p;
  p.lowCutHz = std::clamp(lowCut, 5.f, 1000.f);
  p.highCutHz = std::clamp(highCut, 500.f, 22000.f);
  p.colourFrequencyHz = std::clamp(colourFrequency, 40.f, 20000.f);
  p.colourGainDb = std::clamp(colourGain, -24.f, 24.f);
  p.lowCutQ = p.highCutQ = .70710678f;
  p.colourQ = std::clamp(std::isfinite(colourQ) ? colourQ : .7f, .1f, 20.f);
  return p;
}
} // namespace tfdsp::percussion
