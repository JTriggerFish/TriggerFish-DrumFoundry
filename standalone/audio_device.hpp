#pragma once
#include "plugin_host.hpp"
#include <RtAudio.h>
#include <memory>

namespace drumfoundry::standalone {
struct AudioSettings {
  std::string api, device;
  unsigned sampleRate{48000}, buffer{256};
};
// Device ownership and plugin preparation are separate from audio processing.
// There is no automatic ASIO -> WASAPI or device/rate fallback.
class AudioDevice {
public:
  AudioDevice(PluginHost &plugin, AudioSettings settings);
  ~AudioDevice();
  void List();
  void Start();
  void Stop() noexcept;
  void Reconfigure(unsigned rate, unsigned buffer);
  std::string Status() const;
  std::atomic<unsigned> xruns{}, deviceErrors{};

private:
  void ReportError(const std::string &) noexcept;
  std::string LastError() const;
  static int Callback(void *, void *, unsigned, double, RtAudioStreamStatus,
                      void *) noexcept;
  PluginHost &plugin_;
  AudioSettings settings_;
  std::unique_ptr<RtAudio> audio_;
  unsigned actualBuffer_{}, actualRate_{};
  long driverLatency_{};
  std::string deviceName_;
  bool running_{};
  mutable std::atomic_flag errorLock_ = ATOMIC_FLAG_INIT;
  std::array<char, 512> errorText_{};
};
} // namespace drumfoundry::standalone
