#include "settings_store.hpp"
#include "engine/editing/files.hpp"
#include <fstream>
#include <random>
#ifdef _WIN32
#include <windows.h>
#endif

namespace drumfoundry::standalone {
namespace {
using Json = nlohmann::json;
constexpr auto Schema = "triggerfish.drumfoundry.devices/v1";
unsigned Integer(const Json &value) {
  if (!value.is_number_integer() || value.get<double>() < 0 ||
      value.get<double>() > 384000)
    throw std::runtime_error("Settings rate/buffer must be bounded integers");
  return value.get<unsigned>();
}
// A uniquely owned directory avoids concurrent applications sharing a temp
// file.
struct Temporary {
  std::filesystem::path directory;
  explicit Temporary(const std::filesystem::path &parent) {
    std::random_device random;
    for (unsigned i = 0; i < 32; ++i) {
      auto candidate = parent / (".settings-" + std::to_string(random()));
      if (std::filesystem::create_directory(candidate)) {
        directory = candidate;
        return;
      }
    }
    throw std::runtime_error("Cannot create temporary settings file");
  }
  ~Temporary() {
    std::error_code ignored;
    std::filesystem::remove(directory / "settings.json", ignored);
    std::filesystem::remove(directory, ignored);
  }
};
} // namespace
std::filesystem::path SettingsPath() {
  const auto fits = editing::FitDirectory();
  const auto directory = fits.filename() == "fits"
                             ? fits.parent_path()
                             : fits.parent_path() / "TriggerFish-DrumFoundry";
  return directory / "audio-midi.json";
}
std::optional<ui::DeviceConfiguration>
ReadSettings(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path))
    return std::nullopt;
  if (std::filesystem::file_size(path) > 16384)
    throw std::runtime_error("Audio/MIDI settings file exceeds 16 KB");
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
    throw std::runtime_error("Cannot read audio/MIDI settings");
  const auto json = Json::parse(stream);
  if (json.at("schema") != Schema)
    throw std::runtime_error("Unsupported audio/MIDI settings schema");
  ui::DeviceConfiguration config{
      json.at("api").get<std::string>(), json.at("device").get<std::string>(),
      json.at("midi").get<std::string>(), Integer(json.at("rate")),
      Integer(json.at("buffer"))};
  ui::ValidateDeviceConfiguration(config);
  return config;
}
void WriteSettings(const std::filesystem::path &path,
                   const ui::DeviceConfiguration &config) {
  ui::ValidateDeviceConfiguration(config);
  const Json json{{"schema", Schema},        {"api", config.api},
                  {"device", config.device}, {"midi", config.midi},
                  {"rate", config.rate},     {"buffer", config.buffer}};
  const auto text = json.dump(2) + "\n";
  if (text.size() > 16384)
    throw std::runtime_error("Audio/MIDI settings file exceeds 16 KB");
  const auto parent = std::filesystem::absolute(path).parent_path();
  std::filesystem::create_directories(parent);
  Temporary temp(parent);
  const auto file = temp.directory / "settings.json";
  std::ofstream stream(file, std::ios::binary);
  stream << text;
  stream.close();
  if (!stream)
    throw std::runtime_error("Could not finish writing audio/MIDI settings");
#ifdef _WIN32
  if (!MoveFileExW(file.c_str(), path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error(
        "Cannot replace audio/MIDI settings: Windows error " +
        std::to_string(GetLastError()));
#else
  std::filesystem::rename(file, path);
#endif
}
ui::DeviceConfiguration MergeSettings(ui::DeviceConfiguration saved,
                                      const ui::DeviceConfiguration &cli,
                                      unsigned overrides) {
  if (overrides & Api) {
    if (saved.api != cli.api)
      saved.device.clear(); // Never carry a device name into a different API.
    saved.api = cli.api;
  }
  if (overrides & Device)
    saved.device = cli.device;
  if (overrides & Midi)
    saved.midi = cli.midi;
  if (overrides & Rate)
    saved.rate = cli.rate;
  if (overrides & Buffer)
    saved.buffer = cli.buffer;
  return saved;
}
} // namespace drumfoundry::standalone
