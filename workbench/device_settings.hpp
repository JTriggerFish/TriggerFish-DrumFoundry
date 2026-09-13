#pragma once
#include <stdexcept>
#include <string>

namespace drumfoundry::ui {
// Host preferences, not synthesis parameters or part of a fit document.
struct DeviceConfiguration {
  std::string api, device, midi{"all"};
  unsigned rate{48000}, buffer{256};
};
inline void ValidateDeviceConfiguration(const DeviceConfiguration &config) {
  if (config.api.empty() || config.device.empty())
    throw std::invalid_argument("Choose an audio API and output device first");
  if (config.rate < 8000 || config.rate > 384000 || config.buffer < 16 ||
      config.buffer > 16384)
    throw std::invalid_argument("Unsupported sample rate or buffer size");
  if (config.midi.empty())
    throw std::invalid_argument("Choose a MIDI input, all or none");
}
} // namespace drumfoundry::ui
