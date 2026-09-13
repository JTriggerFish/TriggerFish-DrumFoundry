#pragma once
#include <filesystem>
#include <vector>
namespace drumfoundry::analysis {
enum class Channel { MonoAverage, Left, Right };
struct Audio {
  unsigned sampleRate{}, sourceChannels{1};
  std::vector<float> samples;
};
// Decode without gain matching, filtering or resampling. Channel choice is
// explicit and retained by the workbench; analysis renders at this sample rate.
Audio ReadWave(const std::filesystem::path &, Channel = Channel::MonoAverage);
std::string FileHash(const std::filesystem::path &);
} // namespace drumfoundry::analysis
