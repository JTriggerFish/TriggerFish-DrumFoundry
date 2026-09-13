#include "audio.hpp"
#include <cmath>
#include <samplerate.h>
#include <stdexcept>
namespace drumfoundry::analysis {
std::vector<float> Resample(const Audio &audio, unsigned rate) {
  if (rate < 8000 || rate > 384000 || audio.sampleRate < 8000 ||
      audio.sampleRate > 384000 ||
      audio.samples.size() > std::size_t(audio.sampleRate) * 60)
    throw std::invalid_argument("Invalid audition sample rate or duration");
  if (audio.sampleRate == rate || audio.samples.empty())
    return audio.samples;
  const double ratio = double(rate) / audio.sampleRate;
  std::vector<float> output(
      std::size_t(std::ceil(audio.samples.size() * ratio)) + 256);
  SRC_DATA data{};
  data.data_in = audio.samples.data();
  data.data_out = output.data();
  data.input_frames = long(audio.samples.size());
  data.output_frames = long(output.size());
  data.src_ratio = ratio;
  data.end_of_input = 1;
  const int error = src_simple(&data, SRC_SINC_BEST_QUALITY, 1);
  if (error)
    throw std::runtime_error(src_strerror(error));
  if (data.input_frames_used != data.input_frames)
    throw std::runtime_error("Incomplete audition resampling");
  output.resize(std::size_t(data.output_frames_gen));
  return output;
}
} // namespace drumfoundry::analysis
