#include "editing/files.hpp"
#include "library.hpp"
#include <fstream>
#include <random>
#ifdef _WIN32
#include <windows.h>
#endif

namespace drumfoundry::analysis {
std::filesystem::path LibrarySettingsPath() {
  return editing::FitDirectory().parent_path() / "reference-library.json";
}
std::filesystem::path ReadLibraryRoot(const std::filesystem::path &settings) {
  if (!std::filesystem::exists(settings))
    return {};
  if (std::filesystem::file_size(settings) > 16384)
    throw std::runtime_error("Reference library settings exceed 16 KB");
  std::ifstream stream(settings);
  const auto value = Json::parse(stream);
  if (value.at("schema") != "triggerfish.drumfoundry.reference-library/v1")
    throw std::runtime_error("Unsupported reference library settings");
  const auto root =
      std::filesystem::u8path(value.at("root").get<std::string>());
  if (!root.empty() && !root.is_absolute())
    throw std::runtime_error(
        "Reference library setting must be an absolute folder");
  return root;
}
void SaveLibraryRoot(const std::filesystem::path &root,
                     const std::filesystem::path &settings) {
  if (!root.empty() && !std::filesystem::is_directory(root))
    throw std::runtime_error("Choose an existing reference library folder");
  const auto absolute = root.empty() ? root : std::filesystem::canonical(root);
  const auto text =
      Json{{"schema", "triggerfish.drumfoundry.reference-library/v1"},
           {"root", absolute.u8string()}}
          .dump(2) +
      "\n";
  if (text.size() > 16384)
    throw std::runtime_error("Reference library settings exceed 16 KB");
  const auto parent = std::filesystem::absolute(settings).parent_path();
  std::filesystem::create_directories(parent);
  std::filesystem::path temporary;
  std::random_device random;
  for (int attempt = 0; attempt < 32; ++attempt) {
    const auto candidate =
        parent / (".reference-library-" + std::to_string(random()));
    if (std::filesystem::create_directory(candidate)) {
      temporary = candidate;
      break;
    }
  }
  if (temporary.empty())
    throw std::runtime_error("Cannot prepare reference library settings");
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
  stream << text;
  stream.close();
  if (!stream)
    throw std::runtime_error("Cannot write reference library settings");
#ifdef _WIN32
  if (!MoveFileExW(file.c_str(), settings.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error("Cannot replace reference library settings");
#else
  std::filesystem::rename(file, settings);
#endif
}
} // namespace drumfoundry::analysis
