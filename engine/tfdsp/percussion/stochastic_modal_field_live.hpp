#pragma once

namespace tfdsp::percussion {
template <std::size_t N>
bool StochasticModalField<N>::RetainState(
    const StochasticModalField &old) noexcept {
  if (this == &old)
    return true;
  if (!old.CanAdoptModalEdit())
    return false;
  // At most one generation retires at once. The host keeps the latest target
  // pending during this 5 ms fade, so rapid drags cannot grow a tail list.
  retiring_.Reset();
  ModalIdentityMap<N> destination;
  for (std::size_t i = 0; i < activeModeCount_; ++i)
    destination.Insert(identity_[i], i);
  for (std::size_t i = 0; i < old.activeModeCount_; ++i)
    if (destination.Find(old.identity_[i]) == N)
      retiring_.Add(old.real_[i], old.imaginary_[i], old.cosineCentre_[i],
                    old.sineCentre_[i], old.radius_[i], old.outputGain_[i],
                    old.band_[i]);
  real_.fill(0.f);
  imaginary_.fill(0.f);
  ModalIdentityMap<N> members, packets;
  std::array<double, N> oldEnergy{}, keptEnergy{}, newWeight{}, totalWeight{};
  std::array<std::size_t, N> source{}, oldPacket{};
  source.fill(N);
  oldPacket.fill(N);
  for (std::size_t i = 0; i < old.activeModeCount_; ++i) {
    members.Insert(old.identity_[i], i);
    auto p = packets.Find(old.packetIdentity_[i]);
    if (p == N) {
      p = i;
      packets.Insert(old.packetIdentity_[i], p);
    }
    oldEnergy[p] += double(old.real_[i]) * old.real_[i] +
                    double(old.imaginary_[i]) * old.imaginary_[i];
  }
  for (std::size_t i = 0; i < activeModeCount_; ++i) {
    source[i] = members.Find(identity_[i]);
    oldPacket[i] = packets.Find(packetIdentity_[i]);
    if (oldPacket[i] != N) {
      const double weight = double(inputGain_[i]) * inputGain_[i];
      totalWeight[oldPacket[i]] += weight;
      if (source[i] == N)
        newWeight[oldPacket[i]] += weight;
    }
    if (source[i] == N)
      continue;
    real_[i] = old.real_[source[i]];
    imaginary_[i] = old.imaginary_[source[i]];
    if (oldPacket[i] != N)
      keptEnergy[oldPacket[i]] +=
          double(real_[i]) * real_[i] + double(imaginary_[i]) * imaginary_[i];
  }
  // Reallocation redistributes a packet's stored energy over its new members;
  // changing density must not add energy or drain a sounding packet. Reserve
  // the new members' share using the prepared, normalized input weights.
  // A new handle is silent until excited; deleting a handle removes its energy.
  for (std::size_t i = 0; i < activeModeCount_; ++i) {
    const auto p = oldPacket[i];
    float scale = 1.f;
    if (p != N && totalWeight[p] > 0) {
      if (keptEnergy[p] > 1.e-30) {
        const double retained =
            std::max(0., 1. - newWeight[p] / totalWeight[p]);
        scale = float(std::sqrt(oldEnergy[p] * retained / keptEnergy[p]));
      }
      if (source[i] == N || keptEnergy[p] <= 1.e-30) {
        const float magnitude =
            float(std::sqrt(oldEnergy[p] / totalWeight[p]) * double(inputGain_[i]));
        real_[i] = magnitude * inputPhaseCosine_[i];
        imaginary_[i] = magnitude * inputPhaseSine_[i];
        source[i] = N;
        scale = 1.f;
      }
    }
    real_[i] *= scale;
    imaginary_[i] *= scale;
    outputGain_[i] = source[i] != N && scale > 1.e-12f
                         ? old.outputGain_[source[i]] / scale
                         : 0.f;
  }
  gainRampRemaining_ = std::max(1u, unsigned(.005f * sampleRate_));
  retiring_.Start(gainRampRemaining_);
  random_ = old.random_;
  oddExchange_ = old.oddExchange_;
  cascade_.RetainHistory(old.cascade_);
  drift_.RetainHistory(old.drift_, source);
  motion_.RetainHistory(old.motion_, source);
  tailDamping_ = old.tailDamping_;
  liveTailDamping_ = old.liveTailDamping_;
  rimContact_.RetainState(old.rimContact_);
  UpdateDamping(old.damping_);
  return true;
}
} // namespace tfdsp::percussion
