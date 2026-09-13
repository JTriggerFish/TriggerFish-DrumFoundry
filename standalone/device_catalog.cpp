#include "device_catalog.hpp"
#include <RtAudio.h>
#include <RtMidi.h>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

namespace drumfoundry::standalone {
namespace {
// Enumerate registry names only. Probing every installed ASIO DLL can display
// missing-hardware vendor dialogs, or interfere with an open DAW.
std::vector<std::string> AsioNames() {
  std::vector<std::string> names;
#ifdef _WIN32
  HKEY key{};
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ASIO", 0,
                    KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
    return names;
  for (DWORD i = 0;; ++i) {
    char name[256];
    DWORD size = sizeof(name);
    const auto result =
        RegEnumKeyExA(key, i, name, &size, nullptr, nullptr, nullptr, nullptr);
    if (result == ERROR_NO_MORE_ITEMS)
      break;
    if (result != ERROR_SUCCESS) {
      RegCloseKey(key);
      throw std::runtime_error("Cannot enumerate ASIO registration");
    }
    names.emplace_back(name, size);
  }
  RegCloseKey(key);
#endif
  return names;
}
} // namespace
std::vector<std::string> AudioApis() {
  std::vector<RtAudio::Api> apis;
  RtAudio::getCompiledApi(apis);
  std::vector<std::string> names;
  for (auto api : apis)
    names.push_back(RtAudio::getApiName(api));
  return names;
}
std::vector<std::string> AudioDevices(const std::string &name) {
  if (name == "asio")
    return AsioNames();
  const auto api = RtAudio::getCompiledApiByName(name);
  if (api == RtAudio::UNSPECIFIED)
    throw std::runtime_error("Unknown audio API");
  RtAudio scan(api);
  std::vector<std::string> names;
  for (auto id : scan.getDeviceIds()) {
    const auto device = scan.getDeviceInfo(id);
    if (device.outputChannels >= 2)
      names.push_back(device.name);
  }
  return names;
}
std::vector<std::string> MidiDevices() {
  try {
    RtMidiIn scan;
    std::vector<std::string> names{"all", "none"};
    for (unsigned i = 0; i < scan.getPortCount(); ++i)
      names.push_back(scan.getPortName(i));
    return names;
  } catch (const RtMidiError &e) {
    throw std::runtime_error(e.getMessage());
  }
}
} // namespace drumfoundry::standalone
