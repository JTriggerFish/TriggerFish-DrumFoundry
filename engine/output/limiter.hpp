#pragma once
#include "peak_detector.hpp"
#include "peak_hold.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace drumfoundry::output {
struct LimiterStatus {
  bool enabled{};
  std::size_t latencySamples{};
  double latencyMs{};
  double reductionDb{};        // Current positive gain reduction.
  double maximumReductionDb{}; // Since ClearMeters; UI latches between polls.
  double inputPeakDb{
      -160}; // Pre-limiter, including estimated intersample peaks.
  double outputPeakDb{-160}; // Sample peak, explicitly not labelled true peak.
  bool invalidInput{};
};

// Optional host-output protection, never inserted into raw Voice/fitting
// output. Prepare and destruction are off-audio-thread.
// Process/Reset/ClearMeters allocate nothing and need externally serialized
// access. Mono/stereo use linked gain.
class Limiter {
public:
  static constexpr double LookaheadSeconds = .001;
  static constexpr double CeilingDb = -1.;
  static constexpr double ReleaseSeconds = .1;
  static constexpr double HoldSeconds = .03;
  // Same explicit reconstruction reserve as the old workbench: quarter-phase
  // sampling can miss a peak by cos(pi/8), plus 0.1 dB FIR margin. No makeup
  // gain.
  static constexpr double ReconstructionReserveDb = .787693081581;

  void Prepare(double sampleRate, std::size_t channels = 1,
               bool enabled = true);
  void Reset() noexcept;
  // Bypass selection requires Prepare while stopped: zero bypass latency, no
  // unsafe sample-drop or PDC change inside a running host callback.
  bool Process(float *const *channels, std::size_t frames) noexcept;
  std::array<float, 2> ProcessFrame(std::array<float, 2> input) noexcept;
  LimiterStatus Status() const noexcept;
  void ClearMeters() noexcept;

private:
  double FollowGain(double peak) noexcept;
  void Observe(double inputPeak, double outputPeak) noexcept;
  std::array<PeakDetector, 2> detectors_;
  std::array<GainAverage, 2> smoothing_;
  PeakHold hold_;
  std::array<std::vector<float>, 2> delay_;
  std::size_t delaySamples_{}, write_{}, channels_{};
  double sampleRate_{48000}, releaseStep_{}, protectedCeiling_{};
  double releaseGain_{1}, gain_{1}, minimumGain_{1}, inputPeak_{},
      outputPeak_{};
  bool enabled_{}, invalidInput_{};
};
} // namespace drumfoundry::output
