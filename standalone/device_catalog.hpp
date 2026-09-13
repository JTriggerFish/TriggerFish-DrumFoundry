#pragma once
#include <string>
#include <vector>
namespace drumfoundry::standalone {
std::vector<std::string> AudioApis();
std::vector<std::string> AudioDevices(const std::string &api);
std::vector<std::string> MidiDevices();
} // namespace drumfoundry::standalone
