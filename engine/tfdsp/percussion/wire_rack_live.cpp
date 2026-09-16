#include "wire_rack.hpp"
#include <algorithm>
#include <cmath>

namespace tfdsp::percussion {
void WireRack::SetLiveControls(const WireRackParameters &source) noexcept {
  const auto p = PrepareWireRackLiveParameters(parameters_.sampleRate, source);
  const float rate = parameters_.sampleRate;
  sensitivity_.Target(p.sensitivity, rate);
  threshold_.Target(p.threshold, rate);
  motion_.Target(p.motionCoefficient, rate);
  attack_.Target(p.attackCoefficient, rate);
  release_.Target(p.releaseCoefficient, rate);
  noiseMix_.Target(p.noiseMix, rate);
  modalMix_.Target(p.modalMix, rate);
  if (p.noiseTiltDb != parameters_.controls.noiseTiltDb)
    tilt_.SetLiveTilt(p.noiseTiltDb, 3500.f);
  parameters_.controls = p;
}

bool WireRack::AdoptPrepared(const WireRackPreparedParameters &p) noexcept {
  if (!CanAdoptPrepared() || p.sampleRate != parameters_.sampleRate ||
      !p.activeModeCount || p.activeModeCount > WireRackModeCount)
    return false;
  const auto kept = std::min(p.activeModeCount, parameters_.activeModeCount);
  double energy = 0, keptEnergy = 0;
  retiring_.Reset();
  for (std::size_t i = 0; i < parameters_.activeModeCount; ++i) {
    const double e = double(real_[i]) * real_[i] + double(imaginary_[i]) * imaginary_[i];
    energy += e;
    if (i < kept)
      keptEnergy += e;
    else
      retiring_.Add(real_[i], imaginary_[i], parameters_.cosine[i],
                    parameters_.sine[i], parameters_.radius[i], outputGain_[i].Current(), 0);
  }
  // Slots are stable identities. Spread stored energy over newly added slots;
  // removed slots donate to survivors. Observation ramps compensate that
  // remap, while retiring slots fade without re-excitation or feedback.
  const float scale = keptEnergy > 1.e-30
      ? float(std::sqrt(energy * (double(kept) / p.activeModeCount) / keptEnergy)) : 1.f;
  const float magnitude = float(std::sqrt(energy / p.activeModeCount));
  for (std::size_t i = 0; i < p.activeModeCount; ++i) {
    float observed = 0;
    if (i < kept && keptEnergy > 1.e-30) {
      real_[i] *= scale;
      imaginary_[i] *= scale;
      if (scale > 1.e-12f)
        observed = outputGain_[i].Current() / scale;
    } else {
      real_[i] = magnitude * p.inputPhaseCosine[i];
      imaginary_[i] = magnitude * p.inputPhaseSine[i];
    }
    outputGain_[i].Reset(energy > 0 ? observed : p.modeOutputGain[i]);
    outputGain_[i].Target(p.modeOutputGain[i], p.sampleRate);
  }
  for (std::size_t i = p.activeModeCount; i < WireRackModeCount; ++i)
    real_[i] = imaginary_[i] = 0.f;
  retiring_.Start(std::max(1u, unsigned(.005f * p.sampleRate)));
  inputGain_.Target(1.f / std::sqrt(float(p.activeModeCount)), p.sampleRate);
  // Scalar automation may be newer than this off-thread snapshot. Keep its
  // targets and ramps, along with follower/filter memory and RNG history.
  const auto controls = parameters_.controls;
  parameters_ = p;
  parameters_.controls = controls;
  return true;
}
} // namespace tfdsp::percussion
