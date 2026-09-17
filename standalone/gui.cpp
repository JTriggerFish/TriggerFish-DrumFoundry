#include "gui.hpp"
#include "drumfoundry/version.hpp"
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
    c.setColor(ui::colours::Overlay);
    c.fill(0, 0, shade.width(), shade.height());
  };
  auto bridge = session.Connect();
  // Exercise the actual CLAP output/meter path without acquiring any
  // hardware.
  visage::EventTimer smokeAudio;
  std::array<float, 1024> smokePcm{};
  unsigned smokeFrames = 0;
  if (smoke) {
    host.Prepare(48000, 512);
    bridge.audioRunning = [] { return true; };
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
  window.setTitle(std::string("TriggerFish DrumFoundry ") + Version);
  window.setMinimumDimensions(900, 600);
  ui::Workbench editor(std::move(bridge));
  editor.textSizeChanged = [&] {
    shade.setPalette(editor.palette());
    ui::NativeFonts(settings);
    settings.resized();
  };
  if (!settingsError.empty())
    editor.Error(settingsError);
  settings.error = [&](const std::string &error) { editor.Error(error); };
  window.addChild(&editor);
  window.setPalette(
      editor.palette()); // Root-owned popups use this editor's text size.
  window.addChild(&shade, false);
  shade.addChild(&settings);
  editor.textSizeChanged();
  settings.onVisibilityChange() = [&] {
    if (!settings.isVisible())
      shade.setVisible(false);
  };
  settings.setOnTop(true);
  window.onResize() = [&] {
    editor.setBounds(window.localBounds());
    shade.setBounds(window.localBounds());
    settings.setBounds((window.width() - 640) / 2,
                       (window.height() - 520) / 2, 640, 520);
  };
  // Saved selections and --device both start on launch. Use the same guarded
  // path as Apply so driver errors are visible inside Settings as well.
  const bool started = ShouldStartDevices(config, smoke) && settings.Apply();
  if (!smoke && !started) {
    shade.setVisible(true);
    settings.setVisible(true);
  }
  std::unique_ptr<GuiCapture> capture;
  if (smoke)
    capture = std::make_unique<GuiCapture>(window, editor, shade, settings);
  window.show(smoke ? 2400 : 1440, smoke ? 1200 : 900);
  window.runEventLoop();
  window.setPalette(
      nullptr); // Release the borrowed palette before editor destruction.
  smokeAudio.stopTimer();
  if (smoke)
    host.Stop();
  if (smoke && !capture->Complete())
    throw std::runtime_error("UI screenshot did not complete");
}
} // namespace drumfoundry::standalone
