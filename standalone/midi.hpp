#pragma once
#include "plugin_host.hpp"
#include <RtMidi.h>
#include <memory>

namespace drumfoundry::standalone {
// Each RtMidi callback has its own SPSC queue; inputs never contend on a lock.
class MidiInputs {
public:
  static void List();
  explicit MidiInputs(PluginHost &host, const std::string &selection);
  ~MidiInputs();

private:
  std::vector<std::unique_ptr<RtMidiIn>> inputs_;
};
} // namespace drumfoundry::standalone
