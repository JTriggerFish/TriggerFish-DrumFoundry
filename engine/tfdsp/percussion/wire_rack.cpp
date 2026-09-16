#include "wire_rack.hpp"

#include <algorithm>
#include <cmath>

namespace tfdsp::percussion {
void WireRack::Prepare(const float sampleRate,
                       const WireRackParameters &parameters) {
  Prepare(PrepareWireRackParameters(sampleRate, parameters));
}

void WireRack::Prepare(const WireRackPreparedParameters &parameters) {
  parameters_ = parameters;
  tilt_.Prepare(parameters.sampleRate);
  Reset();
}

void WireRack::Reset() noexcept {
  real_.fill(0.f);
  imaginary_.fill(0.f);
  retiring_.Reset();
  const auto &p = parameters_.controls;
  sensitivity_.Reset(p.sensitivity);
  threshold_.Reset(p.threshold);
  motion_.Reset(p.motionCoefficient);
  attack_.Reset(p.attackCoefficient);
  release_.Reset(p.releaseCoefficient);
  noiseMix_.Reset(p.noiseMix);
  modalMix_.Reset(p.modalMix);
  tilt_.SetTilt(p.noiseTiltDb, 3500.f);
  inputGain_.Reset(1.f / std::sqrt(float(parameters_.activeModeCount)));
  for (std::size_t i = 0; i < WireRackModeCount; ++i)
    outputGain_[i].Reset(parameters_.modeOutputGain[i]);
  motionLowpass_ = contactEnvelope_ = 0.f;
  tilt_.Reset();
  random_.Seed(parameters_.seed);
}

void WireRack::Seed(const std::uint32_t seed) noexcept {
  random_.Seed(seed == 0 ? parameters_.seed : seed);
}

float WireRack::Process(float bodyMotion) noexcept {
  bodyMotion = tfdsp::FiniteNormalOrZero(bodyMotion);
  motionLowpass_ +=
      motion_.Next() * (bodyMotion - motionLowpass_);
  motionLowpass_ = tfdsp::FiniteNormalOrZero(motionLowpass_);
  const float motion = bodyMotion - motionLowpass_;
  const float target = std::max(0.f, std::abs(motion) - threshold_.Next());
  const float attack = attack_.Next(), release = release_.Next();
  contactEnvelope_ = target > contactEnvelope_
                         ? attack * contactEnvelope_ + (1.f - attack) * target
                         : release * contactEnvelope_;
  contactEnvelope_ = tfdsp::FiniteNormalOrZero(contactEnvelope_);
  const float linearContact = sensitivity_.Next() * contactEnvelope_;
  // Wire contact grows with both the number of touching strands and their
  // individual force. Squaring the normalized follower gives a smooth onset
  // without introducing a second trigger or a delayed noise burst.
  const float contact = linearContact * linearContact;
  const float noise = tilt_.Process(1.7320508075688772f * random_.Bipolar());
  const float drive = contact * noise;
  // Continuous noise drive: contact controls sqrt(energy/second).
  // The audible noise port stays in audio units; only modal injection uses dt.
  const float modalDrive = drive / std::sqrt(parameters_.sampleRate);

  const float inputGain = inputGain_.Next();
  for (std::size_t mode = 0; mode < parameters_.activeModeCount; ++mode) {
    const float priorReal = real_[mode];
    const float priorImaginary = imaginary_[mode];
    real_[mode] =
        parameters_.radius[mode] * (parameters_.cosine[mode] * priorReal -
                                    parameters_.sine[mode] * priorImaginary);
    imaginary_[mode] =
        parameters_.radius[mode] * (parameters_.sine[mode] * priorReal +
                                    parameters_.cosine[mode] * priorImaginary);
  }
  float modal = 0.f;
  for (std::size_t mode = 0; mode < parameters_.activeModeCount; ++mode) {
    const float force = inputGain * modalDrive;
    real_[mode] = tfdsp::FiniteNormalOrZero(
        real_[mode] + parameters_.inputPhaseCosine[mode] * force);
    imaginary_[mode] = tfdsp::FiniteNormalOrZero(
        imaginary_[mode] + parameters_.inputPhaseSine[mode] * force);
    modal += outputGain_[mode].Next() * real_[mode];
  }
  return tfdsp::FiniteNormalOrZero(
      noiseMix_.Next() * drive + modalMix_.Next() * (modal + retiring_.Process({})));
}

float WireRack::StoredEnergy() const noexcept {
  float result = 0.f;
  for (std::size_t mode = 0; mode < parameters_.activeModeCount; ++mode)
    result += real_[mode] * real_[mode] + imaginary_[mode] * imaginary_[mode];
  return tfdsp::FiniteNormalOrZero(result);
}

} // namespace tfdsp::percussion
