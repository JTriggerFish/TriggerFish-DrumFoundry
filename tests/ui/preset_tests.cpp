#include "ui/preset_panel.hpp"
#include <chrono>
#include <stdexcept>

void PresetTests(const drumfoundry::editing::Document &d) {
  using namespace drumfoundry;
  auto check = [](bool ok) {
    if (!ok)
      throw std::runtime_error("Preset library regression");
  };
  auto original = d.JsonValue();
  original["reference"] = {{"libraryPath", "fixture/reference.wav"},
                           {"visible", true}};
  const auto first = ui::NewPreset(original, "My sound");
  const auto second = ui::NewPreset(first, "My sound");
  check(first.at("id") != second.at("id"));
  check(first.at("parentId") == original.at("id"));
  check(second.at("parentId") == first.at("id"));
  check(second.at("instrument") == original.at("instrument"));
  check(second.at("reference") == original.at("reference"));
  check(second.at("controls") == original.at("controls"));
  bool rejected = false;
  try {
    ui::NewPreset(original, " \t");
  } catch (const std::exception &) {
    rejected = true;
  }
  check(rejected);
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("drumfoundry-test-" + first.at("id").get<std::string>());
  check(std::filesystem::create_directory(directory));
  editing::WriteNewFit(directory / "first.json", first);
  editing::WriteNewFit(directory / "second.json", second);
  const auto now = std::filesystem::file_time_type::clock::now();
  std::filesystem::last_write_time(directory / "first.json",
                                   now - std::chrono::hours(1));
  std::filesystem::last_write_time(directory / "second.json", now);
  const auto paths = ui::UserPresets(directory);
  check(paths.size() == 2);
  check(paths[0] == directory / "second.json");
  check(paths[1] == directory / "first.json");
  check(ui::PresetLabel(paths[0]).rfind("My sound · ", 0) == 0);
  check(editing::ReadFit(directory / "first.json").at("name") == "My sound");
  rejected = false;
  try {
    editing::WriteNewFit(directory / "first.json", second);
  } catch (const std::exception &) {
    rejected = true;
  }
  check(rejected);
  const auto nested = directory / "Percussion" / "Metal";
  std::filesystem::create_directories(nested);
  editing::WriteNewFit(nested / "third.JSON", second);
  const auto treePaths = ui::UserPresets(directory);
  check(treePaths.size() == 3);
  visage::PopupMenu menu;
  ui::AddPresetFolders(menu, directory, treePaths);
  check(menu.options()[0].name().toUtf8() == "Percussion");
  const auto &metal = menu.options()[0].options()[0];
  check(metal.name().toUtf8() == "Metal");
  const int selected = metal.options()[0].id() - 1000;
  check(selected >= 0 && treePaths.at(selected) == nested / "third.JSON");
  const auto settings = directory / "folder-settings.json";
  check(ui::UserPresetDirectory(settings) == editing::FitDirectory());
  ui::SetUserPresetDirectory(nested, settings);
  check(ui::UserPresetDirectory(settings) ==
        std::filesystem::canonical(nested));
  ui::SetUserPresetDirectory({}, settings);
  check(ui::UserPresetDirectory(settings) == editing::FitDirectory());
  std::filesystem::remove(settings);
  std::filesystem::remove(nested / "third.JSON");
  std::filesystem::remove(nested);
  std::filesystem::remove(directory / "Percussion");
  std::filesystem::remove(directory / "first.json");
  std::filesystem::remove(directory / "second.json");
  std::filesystem::remove(directory);
}
