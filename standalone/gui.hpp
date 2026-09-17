#pragma once
#include "audio_device.hpp"
#include "ui/settings.hpp"
namespace drumfoundry::standalone {
// Empty device starts silently; settings explicitly open/release audio and
// MIDI.
void RunGui(PluginHost &, const ui::DeviceConfiguration &, bool smoke = false,
            unsigned overrides = 0);
void PluginGuiSmoke(PluginHost &);
// Capture the factory hi-hat editor without devices or user presets.
void DocumentationScreenshot(PluginHost &);
} // namespace drumfoundry::standalone
