#include "gui.hpp"
#include "ui/workbench.hpp"
#include <algorithm>
#include <stdexcept>
#include <visage/app.h>

namespace drumfoundry::standalone {
void RunGui(PluginHost &host, AudioDevice *audio, bool smoke) {
  ui::Bridge bridge;
  bridge.value = [&host](unsigned id) { return host.Value(id); };
  bridge.change = [&host, audio](unsigned id, double value) {
    if (id == 100 || id == 106 || !audio) {
      if (audio)
        audio->Stop();
      host.SetStopped(id, value);
      if (audio)
        audio->Start();
    } else if (!host.controls.Push({false, id, value, {}}))
      throw std::runtime_error("Control queue full");
  };
  bridge.strike = [&host, audio](float velocity, float location) {
    if (!audio)
      throw std::runtime_error("No audio device open in UI inspection mode");
    if (!host.controls.Push({false, 103, location, {}}) ||
        !host.controls.Push(
            {true,
             0,
             0,
             {0x90, 60, uint8_t(std::clamp(int(velocity * 127), 1, 127))}}))
      throw std::runtime_error("Strike queue full");
  };
  bridge.stop = [&host] { host.controls.Push({true, 0, 0, {0xb0, 120, 0}}); };
  bridge.service = [&host, audio] {
    host.Service();
    if (audio && host.restart.exchange(false))
      audio->Start();
  };
  bridge.status = [&host, audio] {
    return audio ? audio->Status() : "UI inspection — audio device not opened";
  };
  visage::ApplicationWindow window;
  window.setTitle("TriggerFish DrumFoundry");
  window.setMinimumDimensions(1000, 640);
  ui::Workbench editor(std::move(bridge));
  window.addChild(&editor);
  window.onResize() = [&] { editor.setBounds(window.localBounds()); };
  visage::EventTimer finish;
  bool captured = false;
  int attempts = 0;
  if (smoke) {
    finish.onTimerCallback() = [&] {
      const auto &shot = window.takeScreenshot();
      if (shot.width() > 0 && shot.height() > 0) {
        shot.save("build/ui-smoke.png");
        captured = true;
      }
      if (captured || ++attempts == 5) {
        finish.stopTimer();
        window.window()->close();
      }
    };
    finish.startTimer(1500);
  }
  window.show(1440, 900);
  window.runEventLoop();
  if (smoke && !captured)
    throw std::runtime_error("UI screenshot did not complete");
}
} // namespace drumfoundry::standalone
