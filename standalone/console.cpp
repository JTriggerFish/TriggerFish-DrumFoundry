#include "console.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace drumfoundry::standalone {
int PresetIndex(const std::string &name) {
  constexpr const char *names[]{"kick",  "snare", "hihat",
                                "crash", "ride",  "gong"};
  for (int i = 0; i < 6; ++i)
    if (name == names[i])
      return i;
  throw std::runtime_error("Preset: kick, snare, hihat, crash, ride or gong");
}
namespace {
void Help() {
  std::cout << "Commands:\n  hit [1..127] | panic | status | quit\n"
            << "  preset kick|snare|hihat|crash|ride|gong\n"
            << "  master -60..0 | hardness 0..1 | implement 0..1 | location "
               "0..1 | mute 0..1\n"
            << "  limiter on|off | buffer 16..16384 | rate 8000..384000\n"
            << "MIDI note-ons use linear velocity. All keys strike the "
               "selected instrument.\n";
}
double Number(std::istringstream &in, double low, double high) {
  double value;
  std::string extra;
  if (!(in >> value) || !std::isfinite(value) || value < low || value > high ||
      (in >> extra))
    throw std::runtime_error("Invalid value/range or extra arguments");
  return value;
}
void Execute(const std::string &line, PluginHost &host, AudioDevice &audio) {
  std::istringstream in(line);
  std::string command;
  in >> command;
  if (command.empty())
    return;
  if (command == "help") {
    Help();
    return;
  }
  if (command == "status") {
    std::cout << audio.Status() << '\n' << host.Status() << '\n';
    return;
  }
  if (command == "hit" || command == "panic") {
    unsigned char velocity = 100;
    if (in >> std::ws && !in.eof()) {
      if (command == "panic")
        throw std::runtime_error("Panic takes no arguments");
      const double value = Number(in, 1, 127);
      if (value != std::floor(value))
        throw std::runtime_error("Integer velocity required");
      velocity = static_cast<unsigned char>(value);
    }
    const Event event{true, 0, 0,
                      command == "hit"
                          ? std::array<uint8_t, 3>{0x90, 60, velocity}
                          : std::array<uint8_t, 3>{0xb0, 120, 0}};
    if (!host.controls.Push(event))
      throw std::runtime_error("Control queue full");
    return;
  }
  if (command == "preset" || command == "limiter") {
    std::string value, extra;
    if (!(in >> value) || (in >> extra))
      throw std::runtime_error("Supply one value");
    const auto setting = command == "preset" ? PresetIndex(value)
                         : value == "on"     ? 1
                         : value == "off"
                             ? 0
                             : throw std::runtime_error("Limiter: on or off");
    audio.Stop();
    host.SetStopped(command == "preset" ? 100 : 106, setting);
    audio.Start();
    std::cout << host.Status() << '\n';
    return;
  }
  if (command == "buffer" || command == "rate") {
    const double value = Number(in, command == "buffer" ? 16 : 8000,
                                command == "buffer" ? 16384 : 384000);
    if (value != std::floor(value))
      throw std::runtime_error("Integer required");
    // Zero means retain the other setting.
    audio.Reconfigure(command == "rate" ? unsigned(value) : 0,
                      command == "buffer" ? unsigned(value) : 0);
    std::cout << audio.Status() << '\n';
    return;
  }
  const std::pair<const char *, clap_id> controls[]{{"master", 105},
                                                    {"hardness", 101},
                                                    {"implement", 102},
                                                    {"location", 103},
                                                    {"mute", 104}};
  for (const auto &control : controls)
    if (command == control.first) {
      const auto value = Number(in, control.second == 105 ? -60 : 0,
                                control.second == 105 ? 0 : 1);
      if (!host.controls.Push({false, control.second, value, {}}))
        throw std::runtime_error("Control queue full");
      return;
    }
  throw std::runtime_error("Unknown command; type help");
}
} // namespace
void Console(PluginHost &host, AudioDevice &audio) {
  Help();
  std::string line;
  while (std::cout << "> " && std::getline(std::cin, line)) {
    if (line == "quit" || line == "exit")
      break;
    try {
      Execute(line, host, audio);
      host.Service();
      if (host.restart.exchange(false))
        audio.Start();
    } catch (const std::exception &e) {
      std::cerr << "ERROR: " << e.what() << '\n';
    }
  }
}
void SmokeTest() {
  PluginHost host;
  for (int preset = 0; preset < 6; ++preset) {
    host.SetStopped(100, preset);
    host.Prepare(48000, 128);
    host.controls.Push({true, 0, 0, {0x90, 60, 100}});
    std::array<float, 256> output{};
    double energy = 0;
    for (int i = 0; i < 100; ++i) {
      host.Process(output.data(), 128);
      for (float v : output) {
        if (!std::isfinite(v) || std::abs(v) > .9f)
          throw std::runtime_error("Invalid protected output");
        energy += v * v;
      }
    }
    if (energy == 0 || host.failures)
      throw std::runtime_error("Silent/failed preset");
    host.Stop();
  }
  std::cout << "Standalone CLAP host: six presets and MIDI-style strikes "
               "passed (no device opened).\n";
}
} // namespace drumfoundry::standalone
