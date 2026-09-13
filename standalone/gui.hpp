#pragma once
#include "audio_device.hpp"
#include "ui/settings.hpp"
namespace drumfoundry::standalone {
// Empty device starts silently; settings explicitly open/release audio and
// MIDI.
void RunGui(PluginHost &, const ui::DeviceConfiguration &, bool smoke = false);
void PluginGuiSmoke(PluginHost &);
} // namespace drumfoundry::standalone
