#pragma once
#include "controls.hpp"
#include "editing/files.hpp"

namespace drumfoundry::ui {
// User presets use the existing fit format/library; saving always creates a
// version.
editing::Json NewPreset(editing::Json document, const std::string &name);
std::vector<std::filesystem::path>
UserPresets(const std::filesystem::path &directory);
std::string PresetLabel(const std::filesystem::path &);
std::filesystem::path UserPresetSettingsPath();
std::filesystem::path UserPresetDirectory(
    const std::filesystem::path &settings = UserPresetSettingsPath());
void SetUserPresetDirectory(
    const std::filesystem::path &,
    const std::filesystem::path &settings = UserPresetSettingsPath());
// File indices match the menu's 1000-based selection IDs at every depth.
void AddPresetFolders(visage::PopupMenu &, const std::filesystem::path &root,
                      const std::vector<std::filesystem::path> &paths);
class PresetPanel : public visage::Frame {
public:
  PresetPanel();
  void Open(const editing::Json &);
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void(const std::string &)> error;

private:
  void Save();
  editing::Json document_;
  visage::TextEditor name_;
  visage::UiButton save_{"Save preset"}, cancel_{"Cancel"};
};
} // namespace drumfoundry::ui
