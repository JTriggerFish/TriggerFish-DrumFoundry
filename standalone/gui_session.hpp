#pragma once
#include "audio_device.hpp"
#include "midi.hpp"
#include "ui/bridge.hpp"
#include "ui/settings.hpp"
namespace drumfoundry::standalone {
// Standalone-only device lifetime. CLAP uses the DAW's audio/MIDI settings.
class GuiSession {
public:
  explicit GuiSession(PluginHost &host) : host_(host) {}
  ~GuiSession() { Stop(); }
  void Apply(const ui::DeviceConfiguration &);
  void Stop();
  ui::Bridge Connect();
  ui::SettingsBridge Settings();

private:
  PluginHost &host_;
  std::unique_ptr<AudioDevice> audio_;
  std::unique_ptr<MidiInputs> midi_;
};
} // namespace drumfoundry::standalone
