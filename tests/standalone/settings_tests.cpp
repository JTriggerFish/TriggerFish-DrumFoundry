#include "standalone/midi_selection.hpp"
#include "standalone/settings_store.hpp"
#include <fstream>
#include <random>
#include <stdexcept>

namespace {
void Require(bool result) {
  if (!result)
    throw std::runtime_error("Device settings regression");
}
template <typename Action> void Reject(Action action) {
  bool rejected = false;
  try {
    action();
  } catch (const std::exception &) {
    rejected = true;
  }
  Require(rejected);
}
} // namespace
int main() {
  using namespace drumfoundry::standalone;
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("drumfoundry-settings-test-" + std::to_string(std::random_device{}()));
  Require(std::filesystem::create_directory(directory));
  const auto path = directory / "audio-midi.json";
  Require(!ReadSettings(path));
  drumfoundry::ui::DeviceConfiguration config{"asio", "MOTU M Series",
                                              "SL GRAND 1", 48000, 128};
  WriteSettings(path, config);
  auto restored = ReadSettings(path).value();
  Require(restored.api == config.api && restored.device == config.device &&
          restored.midi == config.midi && restored.rate == 48000 &&
          restored.buffer == 128);
  config.buffer = 512;
  config.midi = "none";
  WriteSettings(path, config); // Replacement, not just first-run creation.
  Require(ReadSettings(path)->buffer == 512 &&
          ReadSettings(path)->midi == "none");
  config.buffer = 0;
  Reject([&] { WriteSettings(path, config); });
  Require(ReadSettings(path)->buffer == 512);
  drumfoundry::ui::DeviceConfiguration cli{"wasapi", "other", "all", 96000,
                                           256};
  auto merged = MergeSettings(restored, cli, Rate | Midi);
  Require(merged.api == "asio" && merged.device == restored.device &&
          merged.midi == "all" && merged.rate == 96000 && merged.buffer == 128);
  Require(MergeSettings(restored, cli, Api).device.empty());
  Require(MergeSettings(restored, cli, Api | Device).device == "other");
  {
    std::ofstream stream(path);
    stream << "{\"schema\":\"bad\"}";
  }
  Reject([&] { ReadSettings(path); });
  {
    std::ofstream stream(path);
    stream << "{";
  }
  Reject([&] { ReadSettings(path); });
  {
    std::ofstream stream(path);
    stream << std::string(16385, ' ');
  }
  Reject([&] { ReadSettings(path); });
  std::filesystem::remove(path);
  Require(
      std::filesystem::is_empty(directory)); // No abandoned save temp files.
  std::filesystem::remove(directory);

  const std::vector<std::string> names{"SL GRAND 1", "SL GRAND 10", "other"};
  Require(SelectMidiPorts(names, "SL GRAND 1") == std::vector<unsigned>{0});
  Require(SelectMidiPorts(names, "other") == std::vector<unsigned>{2});
  Require(SelectMidiPorts(names, "none").empty());
  Reject([&] { SelectMidiPorts(names, "SL GRAND"); });
  Reject([&] { SelectMidiPorts(names, "missing"); });
  Reject([&] { SelectMidiPorts({"SL GRAND 10"}, "SL GRAND 1", true); });
  std::vector<unsigned> opened;
  const auto warning = OpenMidiPorts(names, "all", [&](unsigned index) {
    if (index == 1)
      throw std::runtime_error("WinMM error 7");
    opened.push_back(index);
  });
  Require(opened == std::vector<unsigned>({0, 2}));
  Require(warning.find("SL GRAND 10") != std::string::npos &&
          warning.find("WinMM error 7") != std::string::npos);
}
