#pragma once
#include "audio.hpp"
#include <functional>
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
} // namespace drumfoundry::analysis
