#include "preset_panel.hpp"
#include <fstream>
#include <random>
#ifdef _WIN32
#include <windows.h>
#endif

namespace drumfoundry::ui {
std::filesystem::path UserPresetSettingsPath() {
  return editing::FitDirectory().parent_path() / "preset-library.json";
}
std::filesystem::path
UserPresetDirectory(const std::filesystem::path &settings) {
  if (!std::filesystem::exists(settings))
    return editing::FitDirectory();
  if (std::filesystem::file_size(settings) > 16384)
    throw std::runtime_error("Preset folder settings exceed 16 KB");
  std::ifstream stream(settings);
  const auto value = editing::Json::parse(stream);
  if (value.at("schema") != "triggerfish.drumfoundry.preset-library/v1")
    throw std::runtime_error("Unsupported preset folder settings");
  const auto root =
      std::filesystem::u8path(value.at("root").get<std::string>());
  if (root.empty())
    return editing::FitDirectory();
  if (!root.is_absolute())
    throw std::runtime_error("Preset folder must be absolute");
  return root;
}
void SetUserPresetDirectory(const std::filesystem::path &root,
                            const std::filesystem::path &settings) {
  if (!root.empty() && !std::filesystem::is_directory(root))
    throw std::runtime_error("Choose an existing user preset folder");
  const auto absolute =
      root.empty() ? root : std::filesystem::canonical(root);
  const auto parent = std::filesystem::absolute(settings).parent_path();
  std::filesystem::create_directories(parent);
  std::filesystem::path temporary;
  std::random_device random;
  for (int i = 0; i < 32; ++i) {
    const auto candidate =
        parent / (".preset-library-" + std::to_string(random()));
    if (std::filesystem::create_directory(candidate)) {
      temporary = candidate;
      break;
    }
  }
  if (temporary.empty())
    throw std::runtime_error("Cannot prepare preset folder settings");
  struct Cleanup {
    std::filesystem::path directory;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove(directory / "settings.json", ignored);
      std::filesystem::remove(directory, ignored);
    }
  } cleanup{temporary};
  const auto file = temporary / "settings.json";
  std::ofstream stream(file, std::ios::binary);
  stream << editing::Json{{"schema",
                           "triggerfish.drumfoundry.preset-library/v1"},
                          {"root", absolute.u8string()}}
                .dump(2)
         << '\n';
  stream.close();
  if (!stream)
    throw std::runtime_error("Cannot write preset folder settings");
#ifdef _WIN32
  if (!MoveFileExW(file.c_str(), settings.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error("Cannot replace preset folder settings");
#else
  std::filesystem::rename(file, settings);
#endif
}
} // namespace drumfoundry::ui
