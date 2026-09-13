#pragma once
#include "plugin_host.hpp"
#include <RtMidi.h>
#include <memory>

namespace drumfoundry::standalone {
// Each RtMidi callback has its own SPSC queue; inputs never contend on a lock.
class MidiInputs {
public:
  static void List();
  explicit MidiInputs(PluginHost &host, const std::string &selection,
                      bool exact = false);
  ~MidiInputs();
  const std::string &Warning() const { return warning_; }
  unsigned Count() const { return static_cast<unsigned>(inputs_.size()); }

private:
  void Open(PluginHost &, unsigned index);
  std::vector<std::unique_ptr<RtMidiIn>> inputs_;
  std::string warning_;
};
} // namespace drumfoundry::standalone
