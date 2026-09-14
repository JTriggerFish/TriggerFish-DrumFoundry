#include "editing/files.hpp"
#include "theme.hpp"
#include <fstream>
#include <random>
#ifdef _WIN32
#include <windows.h>
#endif

namespace drumfoundry::ui {
std::filesystem::path ThemeSettingsPath() {
  return editing::FitDirectory().parent_path() / "theme.json";
}
editing::Json ReadTheme(const std::filesystem::path &path) {
  if (std::filesystem::file_size(path) > 65536)
    throw std::runtime_error("Colour scheme exceeds 64 KB");
  std::ifstream stream(path);
  auto theme = editing::Json::parse(stream);
  ValidateTheme(theme);
  return theme;
}
void SaveTheme(const editing::Json &theme,
               const std::filesystem::path &path) {
  ValidateTheme(theme);
  std::filesystem::create_directories(path.parent_path());
  std::filesystem::path temporary;
  std::random_device random;
  for (int i = 0; i < 32; ++i) {
    const auto candidate =
        path.parent_path() / (".theme-" + std::to_string(random()));
    if (std::filesystem::create_directory(candidate)) {
      temporary = candidate;
      break;
    }
  }
  if (temporary.empty())
    throw std::runtime_error("Cannot prepare colour scheme settings");
  struct Cleanup {
    std::filesystem::path directory;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove(directory / "theme.json", ignored);
      std::filesystem::remove(directory, ignored);
    }
  } cleanup{temporary};
  const auto file = temporary / "theme.json";
  std::ofstream stream(file, std::ios::binary);
  stream << theme.dump(2) << '\n';
  stream.close();
  if (!stream)
    throw std::runtime_error("Cannot save colour scheme settings");
#ifdef _WIN32
  if (!MoveFileExW(file.c_str(), path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error("Cannot replace colour scheme settings");
#else
  std::filesystem::rename(file, path);
#endif
}
} // namespace drumfoundry::ui
