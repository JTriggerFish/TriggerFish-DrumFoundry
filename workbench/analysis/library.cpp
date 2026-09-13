#include "library.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace drumfoundry::analysis {
std::string LibraryRelativePath(const std::filesystem::path &root,
                                const std::filesystem::path &sample) {
  if (root.empty())
    throw std::runtime_error("Choose a reference library folder in Settings");
  const auto relative =
      std::filesystem::weakly_canonical(sample).lexically_relative(
          std::filesystem::weakly_canonical(root));
  if (relative.empty() || relative.is_absolute() || *relative.begin() == "..")
    throw std::runtime_error(
        "Reference is outside the selected library folder");
  return relative.generic_u8string();
}
std::filesystem::path ResolveLibrarySample(const std::filesystem::path &root,
                                           const std::string &relative) {
  const auto path = std::filesystem::u8path(relative);
  for (unsigned char c : relative)
    if (c < 32)
      throw std::runtime_error("Invalid character in reference path");
  // Reject foreign-platform absolute paths and separators too.
  if (relative.empty() || path.is_absolute() || path.has_root_name() ||
      relative.find(':') != std::string::npos ||
      relative.find('\\') != std::string::npos)
    throw std::runtime_error("Reference needs a library-relative path");
  for (const auto &part : path)
    if (part == "..")
      throw std::runtime_error("Reference path leaves its library");
  const auto result = root / path;
  LibraryRelativePath(root, result);
  return result;
}
std::vector<std::filesystem::directory_entry>
LibraryFolder(const std::filesystem::path &root, const std::string &relative) {
  if (root.empty())
    throw std::runtime_error("Choose a reference library folder in Settings");
  const auto directory =
      relative.empty() ? root : ResolveLibrarySample(root, relative);
  std::vector<std::filesystem::directory_entry> entries;
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_symlink())
      continue; // No cycles or external folder traversal.
    auto extension = entry.path().extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    if (entry.is_directory() ||
        (entry.is_regular_file() && extension == ".wav"))
      entries.push_back(entry);
    if (entries.size() > 4096)
      throw std::runtime_error("Reference folder has over 4096 entries; "
                               "organise it into subfolders");
  }
  std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
    if (a.is_directory() != b.is_directory())
      return a.is_directory();
    return a.path().filename() < b.path().filename();
  });
  return entries;
}
Json PortableReference(Json ref, const std::filesystem::path &root) {
  if (!ref.is_object())
    return nullptr;
  if (!ref.contains("libraryPath")) {
    // Existing calibrations use /reference/<folder>/<encoded filename> URLs.
    const auto cell = ref.value("cell", Json::object());
    auto url = cell.value("url", "");
    if (url.rfind("/reference/", 0) == 0) {
      std::string decoded;
      url.erase(0, 11);
      for (std::size_t i = 0; i < url.size(); ++i) {
        if (url[i] == '%' && i + 2 < url.size() &&
            std::isxdigit(static_cast<unsigned char>(url[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(url[i + 2]))) {
          decoded += char(std::stoi(url.substr(i + 1, 2), nullptr, 16));
          i += 2;
        } else
          decoded += url[i];
      }
      ref["libraryPath"] = decoded;
    } else if (!root.empty() && ref.contains("localPath")) {
      ref["libraryPath"] = LibraryRelativePath(
          root,
          std::filesystem::u8path(ref.at("localPath").get<std::string>()));
    }
  }
  ref.erase("localPath");
  if (!ref.contains("visible"))
    ref["visible"] = true;
  return ref;
}
} // namespace drumfoundry::analysis
