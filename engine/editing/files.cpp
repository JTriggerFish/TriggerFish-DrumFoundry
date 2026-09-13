#include "files.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#endif
namespace drumfoundry::editing {
namespace {
Json ReadJson(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream)
    throw std::runtime_error("Cannot open fit: " + path.u8string());
  const auto size = stream.tellg();
  if (size <= 0 || size > 1024 * 1024)
    throw std::runtime_error("Fit must be a JSON document smaller than 1 MB");
  stream.seekg(0);
  std::string text(static_cast<std::size_t>(size), '\0');
  if (!stream.read(text.data(), size))
    throw std::runtime_error("Could not read complete fit");
  return Json::parse(text);
}
} // namespace
Json ReadFit(const std::filesystem::path &path) {
  Document validated;
  validated.Load(ReadJson(path));
  return validated.JsonValue();
}
std::string FitName(const std::filesystem::path &path) {
  return ReadJson(path).value("name", path.stem().u8string());
}
void WriteNewFit(const std::filesystem::path &path, const Json &document) {
  Document validated;
  validated.Load(document);
  const auto text = validated.JsonValue().dump(2) + "\n";
  if (text.size() > 1024 * 1024)
    throw std::runtime_error("Fit exceeds the 1 MB size limit");
#ifdef _WIN32
  const int fd =
      _wopen(path.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
             _S_IREAD | _S_IWRITE);
#else
  const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
#endif
  if (fd < 0)
    throw std::runtime_error(
        "Cannot create fit — choose a writable folder and a new filename: " +
        path.u8string());
  std::size_t written = 0;
  while (written < text.size()) {
#ifdef _WIN32
    const auto count =
        _write(fd, text.data() + written, unsigned(text.size() - written));
#else
    const auto count = write(fd, text.data() + written, text.size() - written);
#endif
    if (count <= 0)
      break;
    written += std::size_t(count);
  }
#ifdef _WIN32
  const int closed = _close(fd);
#else
  const int closed = close(fd);
#endif
  if (written != text.size() || closed != 0) {
    std::error_code ignored;
    std::filesystem::remove(path,
                            ignored); // Only our newly created incomplete file.
    throw std::runtime_error(
        "Could not finish saving fit; incomplete file removed");
  }
}
std::filesystem::path FitDirectory() {
#ifdef _WIN32
  if (const auto *base = std::getenv("LOCALAPPDATA"))
    return std::filesystem::u8path(base) / "TriggerFish" / "DrumFoundry" /
           "fits";
#elif defined(__APPLE__)
  if (const auto *base = std::getenv("HOME"))
    return std::filesystem::u8path(base) / "Library" / "Application Support" /
           "TriggerFish" / "DrumFoundry" / "fits";
#else
  if (const auto *base = std::getenv("XDG_DATA_HOME"))
    return std::filesystem::u8path(base) / "TriggerFish" / "DrumFoundry" /
           "fits";
  if (const auto *base = std::getenv("HOME"))
    return std::filesystem::u8path(base) / ".local" / "share" / "TriggerFish" /
           "DrumFoundry" / "fits";
#endif
  return std::filesystem::temp_directory_path() /
         "TriggerFish-DrumFoundry-fits";
}
} // namespace drumfoundry::editing
