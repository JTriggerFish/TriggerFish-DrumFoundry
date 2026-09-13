#pragma once
#include "audio_device.hpp"
namespace drumfoundry::standalone {
// Null audio is the hardware-free UI inspection mode, not a second audio host.
void RunGui(PluginHost &, AudioDevice *, bool smoke = false);
} // namespace drumfoundry::standalone
