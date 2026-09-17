#include "gui.hpp"
#include "gui_session.hpp"
#include "adapters/clap/plugin.hpp"
#include "drumfoundry/version.hpp"
#include "ui/workbench.hpp"
#include <stdexcept>
#include <visage/app.h>

namespace drumfoundry::standalone {
void DocumentationScreenshot(PluginHost &host) {
  auto &plugin = clap_adapter::Plugin::Get(host.Api());
  plugin.SelectFactory(2); // Public, reference-free Hi-hat.
  auto document = plugin.EditableDocument();
  document["controls"]["event"]["strength"] = 1.0;
  document["controls"]["analysis"]["view"] = {
      {"renderSeconds", 2.5}, {"span", 2.5}};
  plugin.EditDocument(document);
  plugin.SetPreviewStrength(1.0);
  host.Prepare(48000, 512); // Populate the real limiter/latency readouts.
  GuiSession session(host); // No device is opened by construction/Connect.
  auto bridge = session.Connect();
  bridge.status = [] { return std::string("TriggerFish DrumFoundry ") + Version; };
  visage::ApplicationWindow window;
  ui::Workbench editor(std::move(bridge));
  window.addChild(&editor);
  window.setPalette(editor.palette());
  window.setTitle(std::string("TriggerFish DrumFoundry ") + Version);
  window.onResize() = [&] { editor.setBounds(window.localBounds()); };
  window.show(2560, 1440);
  editor.SetControlWidth(1080);
  // Open the inline routing accordion through its normal button callback.
  for (auto *child : editor.children())
    if (child->x() == 16 && child->y() == 60)
      if (auto *button = dynamic_cast<visage::UiButton *>(child))
        button->onToggle().callback(button, false);
  bool captured = false;
  unsigned attempts = 0, readyFrames = 0;
  visage::EventTimer timer;
  timer.onTimerCallback() = [&] {
    if (editor.AnalysisReady() && ++readyFrames >= 2) {
      const auto shot = window.takeScreenshot();
      if (shot.width() > 0 && shot.height() > 0) {
        shot.save("build/ui-documentation.png");
        captured = true;
      }
    }
    if (captured || ++attempts >= 40) {
      timer.stopTimer();
      window.window()->close();
    }
  };
  timer.startTimer(500);
  window.runEventLoop();
  window.setPalette(nullptr);
  host.Stop();
  if (!captured)
    throw std::runtime_error("Documentation screenshot did not complete");
}
} // namespace drumfoundry::standalone
