#include "audio.hpp"
#define DR_WAV_IMPLEMENTATION
#include <algorithm>
#include <cmath>
#include <dr_wav.h>
#include <fstream>
#include <picosha2.h>
#include <stdexcept>
namespace drumfoundry::analysis {
std::string FileHash(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
    throw std::runtime_error("Cannot hash reference file");
  std::vector<unsigned char> hash(picosha2::k_digest_size);
  picosha2::hash256(stream, hash.begin(), hash.end());
  if (stream.bad())
    throw std::runtime_error("Reference read failed while hashing");
  return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}
Audio ReadWave(const std::filesystem::path &path, Channel channel) {
  drwav wave{};
#ifdef _WIN32
  const bool opened = drwav_init_file_w(&wave, path.c_str(), nullptr);
#else
  const bool opened = drwav_init_file(&wave, path.c_str(), nullptr);
#endif
  if (!opened)
    throw std::runtime_error("Cannot decode WAV: " + path.u8string());
  struct Close {
    drwav &wave;
    ~Close() { drwav_uninit(&wave); }
  } close{wave};
  if (!wave.channels || wave.channels > 8 || wave.sampleRate < 8000 ||
      wave.sampleRate > 384000 || !wave.totalPCMFrameCount ||
      wave.totalPCMFrameCount > uint64_t(wave.sampleRate) * 60)
    throw std::runtime_error("Reference WAV must contain 1–8 channels, 8–384 "
                             "kHz, and at most 60 seconds");
  Audio result{wave.sampleRate, wave.channels,
               std::vector<float>(std::size_t(wave.totalPCMFrameCount))};
  std::vector<float> block(4096 * wave.channels);
  std::size_t offset = 0;
  while (offset < result.samples.size()) {
    const auto count = drwav_read_pcm_frames_f32(
        &wave, std::min<std::size_t>(4096, result.samples.size() - offset),
        block.data());
    if (!count)
      throw std::runtime_error("Reference WAV is truncated");
    for (unsigned i = 0; i < count; ++i) {
      float value = 0;
      for (unsigned c = 0; c < wave.channels; ++c)
        if (!std::isfinite(block[i * wave.channels + c]))
          throw std::runtime_error("Reference contains nonfinite samples");
      if (channel == Channel::MonoAverage) {
        for (unsigned c = 0; c < wave.channels; ++c)
          value += block[i * wave.channels + c] / wave.channels;
      } else
        value = block[i * wave.channels +
                      (channel == Channel::Right && wave.channels > 1 ? 1 : 0)];
      result.samples[offset + i] = value;
    }
    offset += std::size_t(count);
  }
  return result;
}
} // namespace drumfoundry::analysis
