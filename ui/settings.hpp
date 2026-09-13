#pragma once
#include "controls.hpp"
#include "workbench/device_settings.hpp"
#include <vector>
#include <visage_ui/scroll_bar.h>

namespace drumfoundry::ui {
struct SettingsBridge {
  std::function<std::vector<std::string>()> apis, midi;
  std::function<std::vector<std::string>(const std::string &)> devices;
  // Empty result means success; otherwise audio may run with a visible warning.
  std::function<std::string(const DeviceConfiguration &)> apply;
  std::function<void()> stop;
};
// Standard Visage controls; device ownership stays outside the editor.
class SettingsPanel : public visage::Frame {
public:
  SettingsPanel(SettingsBridge, DeviceConfiguration);
  void draw(visage::Canvas &) override;
  void resized() override;
  std::function<void(const std::string &)> error;
  bool
  Apply(); // False on failure; errors remain visible in the panel and banner.

private:
  void Choose(visage::UiButton &, std::vector<std::string>,
              std::function<void(std::string)>);
  void Refresh();
  void Guard(const std::function<void()> &);
  void UpdateMessage();
  SettingsBridge bridge_;
  DeviceConfiguration config_;
  visage::UiButton api_, device_, midi_, rate_, buffer_,
      apply_{"Apply & start"}, stop_{"Release device"}, close_{"Close"};
  std::string message_;
  visage::ScrollableFrame messageView_;
  visage::Text messageText_;
  float messageHeight_{};
};
} // namespace drumfoundry::ui
