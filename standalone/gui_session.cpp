#include "gui_session.hpp"
#include "device_catalog.hpp"
#include <algorithm>
#include <stdexcept>
namespace drumfoundry::standalone {
void GuiSession::Stop() {
  midi_.reset();
  audio_.reset();
}
void GuiSession::Apply(const ui::DeviceConfiguration &config) {
  ui::ValidateDeviceConfiguration(config);
  Stop(); // Never compete with the old device for an exclusive ASIO driver.
  try {
    auto audio = std::make_unique<AudioDevice>(
        host_,
        AudioSettings{config.api, config.device, config.rate, config.buffer});
    auto midi = std::make_unique<MidiInputs>(host_, config.midi);
    audio->Start();
    audio_ = std::move(audio);
    midi_ = std::move(midi);
  } catch (const RtMidiError &e) {
    throw std::runtime_error(e.getMessage());
  }
}
ui::SettingsBridge GuiSession::Settings() {
  return {AudioApis, MidiDevices, AudioDevices,
          [this](const auto &config) { Apply(config); }, [this] { Stop(); }};
}
ui::Bridge GuiSession::Connect() {
  ui::Bridge bridge;
  bridge.value = [this](unsigned id) { return host_.Value(id); };
  bridge.change = [this](unsigned id, double value) {
    if (id == 100 || id == 106 || !audio_) {
      if (audio_)
        audio_->Stop();
      host_.SetStopped(id, value);
      if (audio_)
        audio_->Start();
    } else if (!host_.controls.Push({false, id, value, {}}))
      throw std::runtime_error("Control queue full");
  };
  bridge.strike = [this](float velocity, float x) {
    if (!audio_)
      throw std::runtime_error(
          "Choose an audio device in Settings, then Apply & start");
    if (!host_.controls.Push(
            {false, host_.Value(100) == 0 ? 101u : 103u, x, {}}) ||
        !host_.controls.Push(
            {true,
             0,
             0,
             {0x90, 60, uint8_t(std::clamp(int(velocity * 127), 1, 127))}}))
      throw std::runtime_error("Strike queue full");
  };
  bridge.stop = [this] {
    if (!host_.controls.Push({true, 0, 0, {0xb0, 120, 0}}))
      throw std::runtime_error("Control queue full");
  };
  bridge.service = [this] {
    host_.Service();
    if (audio_ && host_.restart.exchange(false))
      audio_->Start();
  };
  bridge.status = [this] {
    return audio_ ? audio_->Status()
                  : "Audio stopped — select a device in Settings";
  };
  return bridge;
}
} // namespace drumfoundry::standalone
