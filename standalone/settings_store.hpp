#pragma once
#include "../workbench/device_settings.hpp"
#include <filesystem>
#include <optional>

namespace drumfoundry::standalone {
enum DeviceOverride : unsigned {
  Api = 1,
  Device = 2,
  Midi = 4,
  Rate = 8,
  Buffer = 16
};
std::filesystem::path SettingsPath();
std::optional<ui::DeviceConfiguration>
ReadSettings(const std::filesystem::path &);
// Atomic replacement: failure leaves an existing settings file intact.
void WriteSettings(const std::filesystem::path &,
                   const ui::DeviceConfiguration &);
ui::DeviceConfiguration MergeSettings(ui::DeviceConfiguration saved,
                                      const ui::DeviceConfiguration &cli,
                                      unsigned overrides);
} // namespace drumfoundry::standalone
