#include "midi.hpp"
#include <iostream>
#include <stdexcept>

namespace drumfoundry::standalone {
void MidiInputs::List() {
  RtMidiIn scan;
  for (unsigned i = 0; i < scan.getPortCount(); ++i)
    std::cout << "MIDI " << i << ": " << scan.getPortName(i) << '\n';
}
MidiInputs::MidiInputs(PluginHost &host, const std::string &selection) {
  if (selection == "none")
    return;
  RtMidiIn scan;
  for (unsigned i = 0; i < scan.getPortCount(); ++i) {
    const auto name = scan.getPortName(i);
    if (selection != "all" && name.find(selection) == std::string::npos)
      continue;
    if (inputs_.size() >= host.midi.size())
      throw std::runtime_error("At most 32 MIDI inputs supported");
    auto input = std::make_unique<RtMidiIn>();
    input->ignoreTypes(true, true, true);
    input->setCallback(
        [](double, std::vector<unsigned char> *message, void *user) {
          if (!message || message->size() != 3 || (*message)[0] < 0x80 ||
              (*message)[0] >= 0xf0)
            return;
          auto &queue = *static_cast<EventQueue<> *>(user);
          queue.Push(
              {true, 0, 0, {(*message)[0], (*message)[1], (*message)[2]}});
        },
        &host.midi[inputs_.size()]);
    input->openPort(i, "DrumFoundry input");
    std::cout << "Listening to MIDI: " << name << '\n';
    inputs_.push_back(std::move(input));
  }
  if (inputs_.empty() && selection != "all")
    throw std::runtime_error("No MIDI input matching: " + selection);
  if (inputs_.empty())
    std::cout << "No MIDI inputs found; manual strikes remain available.\n";
}
MidiInputs::~MidiInputs() {
  for (auto &input : inputs_) {
    input->cancelCallback();
    input->closePort();
  }
}
} // namespace drumfoundry::standalone
