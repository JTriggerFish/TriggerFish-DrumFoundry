#pragma once
#include "audio_device.hpp"

namespace drumfoundry::standalone {
void Console(PluginHost &host, AudioDevice &audio);
int PresetIndex(const std::string &name);
void SmokeTest();
} // namespace drumfoundry::standalone
