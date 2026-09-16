#pragma once

#include "deterministic_random.hpp"
#include "retiring_modal_output.hpp"
#include "spectral_tilt_filter.hpp"
#include "tfdsp/finite_audio.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace tfdsp::percussion {

inline constexpr std::size_t WireRackModeCount = 48;

struct WireRackParameters {
  float sensitivity{1.125f};
  float threshold{.004f};
  float motionHighpassHz{140.f};
  float attackSeconds{.002f};
  float releaseSeconds{.018f};
  float minimumFrequencyHz{900.f};
  float maximumFrequencyHz{15500.f};
  float decaySeconds{.16f};
  float decayTilt{.7f};
  float density{.8f};
  float brightness{.62f};
  float noiseMix{.6f};
  float modalMix{.75f};
  std::uint32_t seed{0x57495245u};
};

struct WireRackLiveParameters {
  float motionCoefficient{};
  float attackCoefficient{};
  float releaseCoefficient{};
  float sensitivity{1.125f};
  float threshold{.004f};
  float noiseTiltDb{};
  float noiseMix{.6f};
  float modalMix{.75f};
};

struct WireRackPreparedParameters {
  std::array<float, WireRackModeCount> cosine{};
  std::array<float, WireRackModeCount> sine{};
  std::array<float, WireRackModeCount> radius{};
  std::array<float, WireRackModeCount> inputPhaseCosine{};
  std::array<float, WireRackModeCount> inputPhaseSine{};
  std::array<float, WireRackModeCount> modeOutputGain{};
  WireRackLiveParameters controls{};
  float sampleRate{48000.f};
  std::size_t activeModeCount{WireRackModeCount};
  std::uint32_t seed{0x57495245u};
};

WireRackPreparedParameters PrepareWireRackParameters(
    float sampleRate, const WireRackParameters &parameters);
WireRackLiveParameters PrepareWireRackLiveParameters(
    float sampleRate, const WireRackParameters &parameters) noexcept;

// A compact snare-wire interaction driven continuously by body motion. A
// motion high-pass rejects static displacement, while a contact
// envelope drives correlated noise and a dense, normalized wire-mode bank.
class WireRack {
public:
  void Prepare(float sampleRate, const WireRackParameters &parameters);
  void Prepare(const WireRackPreparedParameters &parameters);
  void Reset() noexcept;
  void Seed(std::uint32_t seed) noexcept;
  // Smooth follower/colour/mix targets without resetting the contact history.
  void SetLiveControls(const WireRackParameters &parameters) noexcept;
  bool CanAdoptPrepared() const noexcept { return !retiring_.Active(); }
  // Callback-safe geometry adoption. Retry the latest target after retirement;
  // newer scalar automation is never replaced by this prepared snapshot.
  bool AdoptPrepared(const WireRackPreparedParameters &parameters) noexcept;
  float Process(float bodyMotion) noexcept;
  float StoredEnergy() const noexcept;
  float ContactAmount() const noexcept { return contactEnvelope_; }

private:
  std::array<float, WireRackModeCount> real_{};
  std::array<float, WireRackModeCount> imaginary_{};
  WireRackPreparedParameters parameters_{};
  std::array<LiveGain, WireRackModeCount> outputGain_{};
  LiveGain inputGain_, sensitivity_, threshold_, motion_, attack_, release_;
  LiveGain noiseMix_, modalMix_;
  RetiringModalOutput<WireRackModeCount> retiring_;
  DeterministicRandom random_{};
  SpectralTiltFilter tilt_{};
  float motionLowpass_{};
  float contactEnvelope_{};
};

} // namespace tfdsp::percussion
