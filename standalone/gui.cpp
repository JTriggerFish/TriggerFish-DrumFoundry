#include "gui.hpp"
#include "gui_capture.hpp"
#include "gui_session.hpp"
#include "settings_store.hpp"
#include "ui/workbench.hpp"
#include <algorithm>
#include <stdexcept>
#include <visage/app.h>

namespace drumfoundry::standalone {
void RunGui(PluginHost &host, const ui::DeviceConfiguration &cli, bool smoke,
            unsigned overrides) {
  auto config = cli;
  std::string settingsError;
  if (!smoke) {
    try {
      if (const auto saved = ReadSettings(SettingsPath()))
        config = MergeSettings(*saved, cli, overrides);
    } catch (const std::exception &e) {
      settingsError =
          std::string("Could not restore audio/MIDI settings: ") + e.what();
    }
  }
  GuiSession session(host);
  auto settingsBridge = session.Settings();
  if (smoke) {
    // Manual clicks during a screenshot test must remain hardware/file-free.
    settingsBridge.apply = [](const auto &) {
      return std::string("Device-free UI test: opening devices and saving "
                         "settings are disabled.");
    };
    settingsBridge.stop = [] {};
  }
  ui::SettingsPanel settings(std::move(settingsBridge), config);
  ui::NativeFonts(settings);
  visage::Frame shade;
  shade.onDraw() = [&](visage::Canvas &c) {
    c.setColor(0x9905090f);
    c.fill(0, 0, shade.width(), shade.height());
  };
  auto bridge = session.Connect();
  // Exercise the actual CLAP output/meter path without acquiring any hardware.
  visage::EventTimer smokeAudio;
  std::array<float, 1024> smokePcm{};
  unsigned smokeFrames = 0;
  if (smoke) {
    host.Prepare(48000, 512);
    bridge.status = [] {
      return "Device-free UI test — generated audio is discarded";
    };
    smokeAudio.onTimerCallback() = [&] {
      if (smokeFrames % (512 * 280) == 0)
        host.midi[0].Push({true, 0, 0, {0x90, 60, 100}});
      host.Process(smokePcm.data(), 512);
      smokeFrames += 512;
    };
    smokeAudio.startTimer(11);
  }
  bridge.settings = [&] {
    shade.setVisible(true);
    settings.setVisible(true);
  };
  visage::ApplicationWindow window;
  window.setTitle("TriggerFish DrumFoundry");
  window.setMinimumDimensions(1000, 640);
  ui::Workbench editor(std::move(bridge));
  if (!settingsError.empty())
    editor.Error(settingsError);
  settings.error = [&](const std::string &error) { editor.Error(error); };
  window.addChild(&editor);
  window.addChild(&shade, false);
  shade.addChild(&settings);
  settings.onVisibilityChange() = [&] {
    if (!settings.isVisible())
      shade.setVisible(false);
  };
  settings.setOnTop(true);
  window.onResize() = [&] {
    editor.setBounds(window.localBounds());
    shade.setBounds(window.localBounds());
    settings.setBounds((window.width() - 640) / 2, (window.height() - 520) / 2,
                       640, 520);
  };
  // Restored choices do not implicitly acquire an exclusive device.
  if (!smoke && (overrides & Device) && !config.device.empty()) {
    try {
      const auto warning = session.Apply(config);
      if (!warning.empty())
        editor.Error(warning);
    } catch (const std::exception &e) {
      editor.Error(e.what());
    }
  }
  std::unique_ptr<GuiCapture> capture;
  if (smoke)
    capture = std::make_unique<GuiCapture>(window, editor, shade, settings);
  window.show(1440, 900);
  window.runEventLoop();
  smokeAudio.stopTimer();
  if (smoke)
    host.Stop();
  if (smoke && !capture->Complete())
    throw std::runtime_error("UI screenshot did not complete");
}
} // namespace drumfoundry::standalone
