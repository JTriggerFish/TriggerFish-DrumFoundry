#pragma once
#include "audio.hpp"
#include <functional>
#include <memory>
#include <string>
namespace drumfoundry::analysis {
struct Transform {
  unsigned size{4096}, hop{512};
  std::string window{"hann"};
};
struct Spectrogram {
  unsigned sampleRate{}, size{}, hop{}, frames{}, bins{};
  std::vector<float> db; // time-major, centred frames, peak-amplitude dBFS/bin
  float maximumDb{-180};
  float At(double seconds, double frequency) const;
  // Display downsampling: retain narrow ridges/transients within a pixel.
  // This is a maximum of existing dBFS bins, never a normalization or a loss.
  float Peak(double timeLow, double timeHigh, double frequencyLow,
             double frequencyHigh) const;
};
Spectrogram Analyze(const Audio &, const Transform &,
                    const std::function<bool()> &cancel = {});
// Reuses FFT/window storage. Emits only new centred frames whose right-hand
// samples are available; Finish permits zero-padding at the true endpoint.
class SpectrumStream {
public:
  SpectrumStream(unsigned rate, Transform);
  ~SpectrumStream();
  Spectrogram Next(const std::vector<float> &samples, bool finish = false,
                   const std::function<bool()> &cancel = {});
  unsigned Frames() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace drumfoundry::analysis
