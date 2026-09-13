#include "gui.hpp"
#include "gui_session.hpp"
#include "ui/workbench.hpp"
#include <algorithm>
#include <stdexcept>
#include <visage/app.h>

namespace drumfoundry::standalone {
void RunGui(PluginHost &host, const ui::DeviceConfiguration &config,
            bool smoke) {
  GuiSession session(host);
  ui::SettingsPanel settings(session.Settings(), config);
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
    settings.setBounds((window.width() - 640) / 2, (window.height() - 440) / 2,
                       640, 440);
  };
  if (!smoke && !config.device.empty()) {
    try {
      session.Apply(config);
    } catch (const std::exception &e) {
      editor.Error(e.what());
    }
  }
  visage::EventTimer finish;
  bool captured = false;
  int attempts = 0;
  int captureStage = 0;
  if (smoke) {
    finish.onTimerCallback() = [&] {
      if (!editor.AnalysisReady()) {
        if (++attempts >= 40) {
          finish.stopTimer();
          window.window()->close();
        }
        return;
      }
      const auto &shot = window.takeScreenshot();
      if (shot.width() > 0 && shot.height() > 0) {
        if (captureStage == 0) {
          shot.save("build/ui-smoke.png");
          shade.setVisible(true);
          settings.setVisible(true);
        } else if (captureStage == 2) {
          shot.save("build/ui-settings-smoke.png");
          settings.setVisible(false);
          editor.OpenRouting();
        } else if (captureStage == 4) {
          shot.save("build/ui-routing-smoke.png");
          captured = true;
        }
        ++captureStage;
      }
      if (captured || ++attempts >= 44) {
        finish.stopTimer();
        window.window()->close();
      }
    };
    finish.startTimer(1500);
  }
  window.show(1440, 900);
  window.runEventLoop();
  smokeAudio.stopTimer();
  if (smoke)
    host.Stop();
  if (smoke && !captured)
    throw std::runtime_error("UI screenshot did not complete");
}
} // namespace drumfoundry::standalone
