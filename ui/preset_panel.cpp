#include "preset_panel.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>

namespace drumfoundry::ui {
editing::Json NewPreset(editing::Json document, const std::string &name) {
  if (name.find_first_not_of(" \t\r\n") == std::string::npos)
    throw std::runtime_error("Please enter a preset name.");
  const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  document["parentId"] = document.value("id", "");
  document["id"] = "native-" + std::to_string(stamp) + "-" +
                   std::to_string(std::random_device{}());
  document["name"] = name;
  return document;
}
std::vector<std::filesystem::path>
UserPresets(const std::filesystem::path &directory) {
  struct Entry {
    std::filesystem::path path;
    std::filesystem::file_time_type modified;
  };
  std::vector<Entry> entries;
  unsigned visited = 0;
  if (std::filesystem::exists(directory)) {
    for (auto it = std::filesystem::recursive_directory_iterator(directory);
         it != std::filesystem::recursive_directory_iterator(); ++it) {
      if (++visited > 4096 || it.depth() >= 16)
        throw std::runtime_error("Preset library is too large (4096 entries "
                                 "/ 16 folder levels maximum)");
      if (it->is_symlink()) {
        it.disable_recursion_pending();
        continue;
      }
      auto extension = it->path().extension().u8string();
      std::transform(extension.begin(), extension.end(), extension.begin(),
                     [](unsigned char c) { return char(std::tolower(c)); });
      if (it->is_regular_file() && extension == ".json")
        entries.push_back({it->path(), it->last_write_time()});
    }
  }
  // Snapshot timestamps once: external saves must not change the comparator
  // during sorting, and opening the menu should avoid repeated disk queries.
  std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
    return a.modified == b.modified ? a.path > b.path
                                    : a.modified > b.modified;
  });
  std::vector<std::filesystem::path> paths;
  paths.reserve(entries.size());
  for (const auto &entry : entries)
    paths.push_back(entry.path);
  return paths;
}
std::string PresetLabel(const std::filesystem::path &path) {
  const auto name = editing::FitName(path);
  const auto fileTime = std::filesystem::last_write_time(path);
  const auto time = std::chrono::system_clock::to_time_t(
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          fileTime - decltype(fileTime)::clock::now() +
          std::chrono::system_clock::now()));
  std::ostringstream label;
  label << name;
  if (const auto *local = std::localtime(&time))
    label << " · " << std::put_time(local, "%Y-%m-%d %H:%M:%S");
  return label.str();
}
PresetPanel::PresetPanel() {
  addChild(&name_);
  addChild(&save_);
  addChild(&cancel_);
  name_.setMultiLine(false);
  name_.onEnterKey() = [this] { Save(); };
  name_.onEscapeKey() = [this] { setVisible(false); };
  save_.onToggle() = [this](auto *, bool) { Save(); };
  cancel_.onToggle() = [this](auto *, bool) { setVisible(false); };
}
void PresetPanel::Open(const editing::Json &document) {
  document_ = document;
  name_.setText(document.value("name", "New preset"));
  setVisible(true);
  name_.requestKeyboardFocus();
  name_.selectAll();
}
void PresetPanel::Save() {
  try {
    const auto document = NewPreset(document_, name_.text().toUtf8());
    const auto directory = UserPresetDirectory();
    std::filesystem::create_directories(directory);
    editing::WriteNewFit(directory /
                             (document.at("id").get<std::string>() + ".json"),
                         document);
    // Saving is storage only: it must not restart a ringing live voice.
    setVisible(false);
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void PresetPanel::resized() {
  name_.setBounds(16, 48, width() - 32, 32);
  save_.setBounds(width() - 276, 116, 140, 30);
  cancel_.setBounds(width() - 124, 116, 108, 30);
}
void PresetPanel::draw(visage::Canvas &c) {
  c.setColor(colours::Panel);
  c.roundedRectangle(0, 0, width(), height(), 8);
  Label(c, "SAVE PRESET AS", 16, 12, width() - 32, 24);
  Label(c, "Saves a new version; previous presets are kept.", 16, 84,
        width() - 32, 24);
}
} // namespace drumfoundry::ui
