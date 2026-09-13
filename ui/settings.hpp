#pragma once
#include "controls.hpp"
#include <vector>

namespace drumfoundry::ui {
struct DeviceConfiguration {
  std::string api, device, midi{"all"};
  unsigned rate{48000}, buffer{256};
};
void ValidateDeviceConfiguration(const DeviceConfiguration &);
struct SettingsBridge {
  std::function<std::vector<std::string>()> apis, midi;
  std::function<std::vector<std::string>(const std::string &)> devices;
  std::function<void(const DeviceConfiguration &)> apply;
  std::function<void()> stop;
};
// Standard Visage controls; device ownership stays outside the editor.
class SettingsPanel : public visage::Frame {
public:
  SettingsPanel(SettingsBridge, DeviceConfiguration);
  void draw(visage::Canvas &) override;
  void resized() override;
  std::function<void(const std::string &)> error;
  void Apply();

private:
  void Choose(visage::UiButton &, std::vector<std::string>,
              std::function<void(std::string)>);
  void Refresh();
  void Guard(const std::function<void()> &);
  SettingsBridge bridge_;
  DeviceConfiguration config_;
  visage::UiButton api_, device_, midi_, rate_, buffer_,
      apply_{"Apply & start"}, stop_{"Release device"}, close_{"Close"};
  std::string message_;
};
} // namespace drumfoundry::ui
