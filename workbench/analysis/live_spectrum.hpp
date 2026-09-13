#pragma once
#include <memory>
#include <vector>
namespace drumfoundry::analysis {
// Trailing Hann FFT for monitoring only. Called off audio thread; no gain
// matching.
class LiveSpectrum {
public:
  static constexpr unsigned Size = 8192;
  LiveSpectrum();
  ~LiveSpectrum();
  void Reset(unsigned rate);
  void Update(const float *samples, unsigned count);
  const std::vector<float> &Decibels() const;
  unsigned Rate() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace drumfoundry::analysis
