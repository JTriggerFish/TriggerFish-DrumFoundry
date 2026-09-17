#include "console.hpp"
#include "drumfoundry/version.hpp"
#include "midi.hpp"
#include "settings_store.hpp"
#ifdef DRUMFOUNDRY_UI
#include "gui.hpp"
#endif
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
struct Options {
  drumfoundry::standalone::AudioSettings audio;
  std::string midi{"all"}, preset{"hihat"};
  bool list{}, midiList{}, midiCheck{}, smoke{}, audition{}, gui{}, uiSmoke{},
      pluginGuiSmoke{};
  unsigned seconds{};
  unsigned deviceOverrides{};
};
unsigned Integer(const std::string &text, unsigned low, unsigned high) {
  std::size_t end = 0;
  const auto value = std::stoul(text, &end);
  if (end != text.size() || value < low || value > high)
    throw std::runtime_error("Invalid numeric option: " + text);
  return static_cast<unsigned>(value);
}
Options Parse(int argc, char **argv) {
  Options options;
#ifdef _WIN32
  options.audio.api = "wasapi";
#elif defined(__APPLE__)
  options.audio.api = "core";
#else
  options.audio.api = "alsa";
#endif
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--list")
      options.list = true;
    else if (arg == "--midi-list")
      options.midiList = true;
    else if (arg == "--midi-check")
      options.midiCheck = true;
    else if (arg == "--smoke")
      options.smoke = true;
    else if (arg == "--audition")
      options.audition = true;
    else if (arg == "--gui")
      options.gui = true;
    else if (arg == "--ui-smoke")
      options.uiSmoke = true;
    else if (arg == "--clap-ui-smoke")
      options.pluginGuiSmoke = true;
    else {
      if (++i >= argc)
        throw std::runtime_error("Missing value for " + arg);
      const std::string value = argv[i];
      using namespace drumfoundry::standalone;
      if (arg == "--api")
        options.deviceOverrides |= Api;
      if (arg == "--device")
        options.deviceOverrides |= Device;
      if (arg == "--midi")
        options.deviceOverrides |= Midi;
      if (arg == "--rate")
        options.deviceOverrides |= Rate;
      if (arg == "--buffer")
        options.deviceOverrides |= Buffer;
      if (arg == "--api")
        options.audio.api = value;
      else if (arg == "--device")
        options.audio.device = value;
      else if (arg == "--midi")
        options.midi = value;
      else if (arg == "--preset")
        options.preset = value;
      else if (arg == "--rate")
        options.audio.sampleRate = Integer(value, 8000, 384000);
      else if (arg == "--buffer")
        options.audio.buffer = Integer(value, 16, 16384);
      else if (arg == "--test-seconds")
        options.seconds = Integer(value, 1, 60);
      else
        throw std::runtime_error("Unknown option: " + arg);
    }
  }
  return options;
}
void Help() {
  std::cout
      << "TriggerFish DrumFoundry " << drumfoundry::Version << '\n'
      << "--version: print version without opening devices\n"
      << "--list [--api asio|wasapi|core|alsa] [--device name]\n"
      << "--api asio --device \"MOTU M Series\" [--buffer 128] [--rate 48000]\n"
      << "  [--midi all|none|name] [--preset "
         "kick|snare|hihat|crash|ride|gong]\n"
      << "  [--test-seconds 5] [--audition]\n"
      << "--smoke: hardware-free integration check\n"
      << "--midi-check --midi name: open/close only the selected MIDI input, "
         "no audio\n"
      << "--gui: native editor (./dev.ps1 ui); omit --device to inspect "
         "silently\n"
      << "--ui-smoke: open/capture/close editor without audio hardware\n"
      << "--ui-screenshot: capture the factory hi-hat editor without devices\n"
      << "--clap-ui-smoke: embedded editor lifecycle check (Windows verified)\n"
      << "Without --gui or --test-seconds, runs an interactive control "
         "console.\n"
      << "Timed device tests are silent unless --audition or live MIDI is "
         "used.\n";
}
} // namespace
int main(int argc, char **argv) {
  using namespace drumfoundry::standalone;
  // Device-test diagnostics must appear immediately even when piped by a
  // launcher.
  std::cout << std::unitbuf;
  try {
#ifdef DRUMFOUNDRY_UI
    if (argc == 2 && std::string(argv[1]) == "--ui-screenshot") {
      PluginHost host;
      DocumentationScreenshot(host);
      return 0;
    }
#endif
    if (argc == 2 && std::string(argv[1]) == "--version") {
      std::cout << "TriggerFish DrumFoundry " << drumfoundry::Version << '\n';
      return 0; // No plugin construction or device access.
    }
    if ((argc > 1 && std::string(argv[1]) == "--help")
#ifndef DRUMFOUNDRY_UI
        || argc == 1
#endif
    ) {
      Help();
      return 0;
    }
    auto options = Parse(argc, argv);
#ifdef DRUMFOUNDRY_UI
    if (argc == 1)
      options.gui = true;
#endif
    if (options.smoke) {
      SmokeTest();
      return 0;
    }
    if (options.midiList) {
      MidiInputs::List();
      return 0;
    }
    PluginHost host;
    if (options.midiCheck) {
      if (options.midi == "all" || options.midi == "none")
        throw std::runtime_error(
            "MIDI check requires one explicit --midi name");
      MidiInputs midi(host, options.midi);
      return midi.Warning().empty() && midi.Count() == 1 ? 0 : 1;
    }
#ifdef DRUMFOUNDRY_UI
    if (options.pluginGuiSmoke) {
      PluginGuiSmoke(host);
      return 0;
    }
    if (options.uiSmoke || options.gui) {
      host.SetStopped(100, PresetIndex(options.preset));
      RunGui(host,
             {options.audio.api, options.audio.device, options.midi,
              options.audio.sampleRate, options.audio.buffer},
             options.uiSmoke, options.deviceOverrides);
      return 0;
    }
#else
    if (options.gui || options.uiSmoke || options.pluginGuiSmoke)
      throw std::runtime_error("Build the Visage editor with ./dev.ps1 ui");
#endif
    AudioDevice audio(host, options.audio);
    if (options.list) {
      audio.List();
      MidiInputs::List();
      return 0;
    }
    if (options.audio.device.empty())
      throw std::runtime_error("Choose --device explicitly; use --list first");
    host.SetStopped(100, PresetIndex(options.preset));
    MidiInputs midi(host, options.midi);
    audio.Start();
    std::cout << audio.Status() << '\n' << host.Status() << '\n';
    if (!options.seconds)
      Console(host, audio);
    else {
      const auto started = std::chrono::steady_clock::now();
      for (unsigned tick = 0; tick < options.seconds * 10; ++tick) {
        if (options.audition && tick % 5 == 0)
          host.controls.Push({true, 0, 0, {0x90, 60, 100}});
        std::this_thread::sleep_until(
            started + std::chrono::milliseconds(100 * (tick + 1)));
        host.Service();
      }
    }
    audio.Stop();
    std::cout << audio.Status() << '\n'
              << host.Status() << "\nCallbacks: " << host.callbacks.load()
              << '\n';
    const bool failed = host.failures || audio.deviceErrors ||
                        !host.callbacks || audio.xruns ||
                        host.DroppedEvents() || !midi.Warning().empty();
    return failed ? 1 : 0;
  } catch (const RtMidiError &e) {
    std::cerr << "MIDI ERROR: " << e.getMessage() << '\n';
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return 1;
  }
}
