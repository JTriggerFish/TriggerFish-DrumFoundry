#include "audio_device.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ASIO is process-global in the SDK. Only probe the explicitly selected driver:
// probing every installed driver can open vendor dialogs for absent hardware.
namespace {
std::string asioProbeFilter;
}
bool DrumFoundryAsioProbeAllowed(const char *name) {
  return asioProbeFilter.empty() ||
         std::string(name).find(asioProbeFilter) != std::string::npos;
}
namespace drumfoundry::standalone {
AudioDevice::AudioDevice(PluginHost &plugin, AudioSettings settings)
    : plugin_(plugin), settings_(std::move(settings)) {
  const auto api = RtAudio::getCompiledApiByName(settings_.api);
  if (api == RtAudio::UNSPECIFIED)
    throw std::runtime_error("Unknown/unavailable audio API: " + settings_.api);
  asioProbeFilter = api == RtAudio::WINDOWS_ASIO ? settings_.device : "";
  audio_ = std::make_unique<RtAudio>(
      api,
      [this](RtAudioErrorType, const std::string &text) { ReportError(text); });
  if (audio_->getCurrentApi() != api)
    throw std::runtime_error("Requested audio API unavailable");
  audio_->showWarnings(false);
}
AudioDevice::~AudioDevice() {
  Stop();
  // Backend teardown may report an error; keep its callback storage alive.
  audio_.reset();
}
void AudioDevice::List() {
  for (const auto id : audio_->getDeviceIds()) {
    const auto d = audio_->getDeviceInfo(id);
    if (!d.outputChannels)
      continue;
    std::cout << d.name << " — " << d.outputChannels << " outputs; rates:";
    for (const auto rate : d.sampleRates)
      std::cout << ' ' << rate;
    std::cout << '\n';
  }
}
void AudioDevice::Start() {
  Stop();
  unsigned selected = 0;
  for (const auto id : audio_->getDeviceIds()) {
    const auto d = audio_->getDeviceInfo(id);
    if (d.outputChannels < 2 ||
        d.name.find(settings_.device) == std::string::npos)
      continue;
    if (selected)
      throw std::runtime_error(
          "Device name is ambiguous; supply a longer name");
    if (std::find(d.sampleRates.begin(), d.sampleRates.end(),
                  settings_.sampleRate) == d.sampleRates.end())
      throw std::runtime_error(
          "Selected device does not support the requested rate");
    selected = id;
    deviceName_ = d.name;
  }
  if (!selected)
    throw std::runtime_error("No stereo output matching: " + settings_.device);
  RtAudio::StreamParameters output{selected, 2, 0};
  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_SCHEDULE_REALTIME;
  options.numberOfBuffers = 2;
  options.streamName = "TriggerFish DrumFoundry";
  actualBuffer_ = settings_.buffer;
  if (audio_->openStream(&output, nullptr, RTAUDIO_FLOAT32,
                         settings_.sampleRate, &actualBuffer_, Callback, this,
                         &options))
    throw std::runtime_error("Open audio: " + audio_->getErrorText());
  try {
    actualRate_ = audio_->getStreamSampleRate();
    driverLatency_ = audio_->getStreamLatency();
    plugin_.Prepare(actualRate_, actualBuffer_);
    if (audio_->startStream())
      throw std::runtime_error("Start audio: " + audio_->getErrorText());
    running_ = true;
  } catch (...) {
    Stop();
    throw;
  }
}
void AudioDevice::Stop() noexcept {
  if (!audio_)
    return;
  if (audio_->isStreamRunning())
    audio_->stopStream();
  if (audio_->isStreamOpen())
    audio_->closeStream();
  plugin_.Stop();
  running_ = false;
}
void AudioDevice::Reconfigure(unsigned rate, unsigned buffer) {
  if (!rate)
    rate = settings_.sampleRate;
  if (!buffer)
    buffer = settings_.buffer;
  if (rate < 8000 || rate > 384000 || buffer < 16 || buffer > 16384)
    throw std::runtime_error("Rate must be 8000..384000; buffer 16..16384");
  settings_.sampleRate = rate;
  settings_.buffer = buffer;
  Start();
}
int AudioDevice::Callback(void *output, void *, unsigned frames, double,
                          RtAudioStreamStatus status, void *user) noexcept {
  auto &self = *static_cast<AudioDevice *>(user);
  if (status)
    ++self.xruns;
  self.plugin_.Process(static_cast<float *>(output), frames);
  return 0;
}
std::string AudioDevice::Status() const {
  std::ostringstream out;
  out << (running_ ? "RUNNING | " : "STOPPED | ") << settings_.api << " / "
      << deviceName_ << " | " << actualRate_ << " Hz | buffer " << actualBuffer_
      << " (requested " << settings_.buffer << ") | driver latency "
      << driverLatency_ << " samples | xruns " << xruns.load()
      << " | device errors " << deviceErrors.load();
  if (deviceErrors)
    out << " | " << LastError();
  return out.str();
}
void AudioDevice::ReportError(const std::string &text) noexcept {
  ++deviceErrors;
  if (errorLock_.test_and_set(std::memory_order_acquire))
    return;
  const auto size = std::min(text.size(), errorText_.size() - 1);
  std::copy_n(text.data(), size, errorText_.data());
  errorText_[size] = 0;
  errorLock_.clear(std::memory_order_release);
}
std::string AudioDevice::LastError() const {
  if (errorLock_.test_and_set(std::memory_order_acquire))
    return "Device error (details pending)";
  const auto snapshot = errorText_;
  errorLock_.clear(std::memory_order_release);
  return snapshot.data();
}
} // namespace drumfoundry::standalone
