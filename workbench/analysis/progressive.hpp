#pragma once
#include "spectrum.hpp"
namespace drumfoundry::analysis {
// Delta transport: do not copy a whole growing spectrogram for every refresh.
struct PreviewChunk {
  std::size_t firstSample{};
  unsigned firstFrame{};
  Audio audio;
  Spectrogram spectrum;
  bool complete{};
};
inline void AppendSpectrum(Spectrogram &target, const Spectrogram &chunk) {
  if (!target.frames) {
    target.sampleRate = chunk.sampleRate;
    target.size = chunk.size;
    target.hop = chunk.hop;
    target.bins = chunk.bins;
  }
  target.db.insert(target.db.end(), chunk.db.begin(), chunk.db.end());
  target.frames += chunk.frames;
  target.maximumDb = std::max(target.maximumDb, chunk.maximumDb);
}
} // namespace drumfoundry::analysis
