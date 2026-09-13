#include "gui_session.hpp"
#include "adapters/clap/plugin.hpp"
#include "device_catalog.hpp"
#include "settings_store.hpp"
#include <algorithm>
#include <stdexcept>
namespace drumfoundry::standalone {
void GuiSession::Stop() {
  midi_.reset();
  audio_.reset();
  midiStatus_.clear();
}
std::string GuiSession::Apply(const ui::DeviceConfiguration &config) {
  ui::ValidateDeviceConfiguration(config);
  // Remember the requested input even if disconnected, so retry needs no setup.
  std::string warning;
  try {
    WriteSettings(SettingsPath(), config);
  } catch (const std::exception &e) {
    warning = std::string("Settings not saved: ") + e.what();
  }
  Stop(); // Never compete with the old device for an exclusive ASIO driver.
  auto audio = std::make_unique<AudioDevice>(
      host_,
      AudioSettings{config.api, config.device, config.rate, config.buffer});
  audio->Start();
  audio_ = std::move(audio); // MIDI errors must not destroy working audio.
  std::string midiWarning;
  try {
    midi_ = std::make_unique<MidiInputs>(host_, config.midi, true);
    midiWarning = midi_->Warning();
  } catch (const RtMidiError &e) {
    midiWarning = e.getMessage();
  } catch (const std::exception &e) {
    midiWarning = e.what();
  }
  midiStatus_ =
      " | MIDI: " + std::to_string(midi_ ? midi_->Count() : 0) + " inputs";
  if (!midiWarning.empty()) {
    midiStatus_ += " (open failed; see Settings)";
    if (!warning.empty())
      warning += "\n";
    warning += "Audio running. " + midiWarning;
  }
  return warning;
}
ui::SettingsBridge GuiSession::Settings() {
  return {AudioApis, MidiDevices, AudioDevices,
          [this](const auto &config) { return Apply(config); },
          [this] { Stop(); }};
}
ui::Bridge GuiSession::Connect() {
  ui::Bridge bridge;
  auto &plugin = clap_adapter::Plugin::Get(host_.Api());
  bridge.velocity = [&plugin] { return plugin.PreviewStrength(); };
  bridge.setVelocity = [&plugin](double v) { plugin.SetPreviewStrength(v); };
  bridge.sampleRate = [&plugin] { return plugin.AuditionRate(); };
  plugin.ReadOutput(nullptr, 0);
  plugin.ReadVoice(nullptr, 0);
  bridge.readVoice = [&plugin](float *pcm, unsigned count) {
    return plugin.ReadVoice(pcm, count);
  };
  bridge.readOutput = [&plugin](float *pcm, unsigned count) {
    return plugin.ReadOutput(pcm, count);
  };
  bridge.play = [&plugin](auto pcm, unsigned rate, double gain) {
    if (!plugin.Audition(std::move(pcm), rate, gain))
      throw std::runtime_error("Select an audio device in Settings; wait for "
                               "the render if its rate changed");
  };
  bridge.document = [&plugin] { return plugin.EditableDocument(); };
  bridge.selectFactory = [this, &plugin](unsigned index) {
    plugin.SelectFactory(index);
    if (audio_) {
      audio_->Stop();
      audio_->Start();
    }
  };
  bridge.presentation = [&plugin](const auto &ref, const auto &analysis) {
    plugin.EditPresentation(ref, analysis);
  };
  bridge.revision = [&plugin] { return plugin.DocumentRevision(); };
  bridge.layout = [&plugin](const auto &positions) {
    plugin.EditLayout(positions);
  };
  bridge.applyDocument = [this, &plugin](const auto &document) {
    // Validation occurs before stopping a working stream. Publication is main
    // thread only; audio owns its existing Voice until Stop joins its callback.
    plugin.EditDocument(document);
    if (audio_) {
      audio_->Stop();
      audio_->Start();
    }
  };
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
  bridge.strike = [this, &plugin](float velocity, float x) {
    if (!audio_)
      throw std::runtime_error(
          "Choose an audio device in Settings, then Apply & start");
    if (!plugin.QueueStrike(velocity, x))
      throw std::runtime_error("Strike queue full");
  };
  bridge.stop = [this] {
    if (!host_.controls.Push({true, 0, 0, {0xb0, 120, 0}}))
      throw std::runtime_error("Control queue full");
  };
  bridge.service = [this, &plugin] {
    host_.Service();
    plugin.PrepareEditorPreset();
    if (audio_ && host_.restart.exchange(false))
      audio_->Start();
  };
  bridge.status = [this] {
    return audio_ ? audio_->Status() + midiStatus_
                  : "Audio stopped — select a device in Settings";
  };
  return bridge;
}
} // namespace drumfoundry::standalone
