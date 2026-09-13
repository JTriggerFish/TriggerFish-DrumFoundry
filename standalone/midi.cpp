#include "midi.hpp"
#include "midi_selection.hpp"
#include <iostream>
#include <stdexcept>

namespace drumfoundry::standalone {
void MidiInputs::List() {
  RtMidiIn scan;
  for (unsigned i = 0; i < scan.getPortCount(); ++i)
    std::cout << "MIDI " << i << ": " << scan.getPortName(i) << '\n';
}
MidiInputs::MidiInputs(PluginHost &host, const std::string &selection,
                       bool exact) {
  if (selection == "none")
    return;
  RtMidiIn scan;
  std::vector<std::string> names;
  for (unsigned i = 0; i < scan.getPortCount(); ++i)
    names.push_back(scan.getPortName(i));
  warning_ = OpenMidiPorts(
      names, selection,
      [&](unsigned i) {
        try {
          Open(host, i);
        } catch (const RtMidiError &e) {
          throw std::runtime_error(e.getMessage());
        }
        std::cout << "Listening to MIDI: " << names[i] << '\n';
      },
      exact);
  if (!warning_.empty())
    std::cerr << warning_ << '\n';
  if (inputs_.empty() && warning_.empty())
    std::cout << "No MIDI inputs found; manual strikes remain available.\n";
}
void MidiInputs::Open(PluginHost &host, unsigned i) {
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
        queue.Push({true, 0, 0, {(*message)[0], (*message)[1], (*message)[2]}});
      },
      &host.midi[inputs_.size()]);
  input->openPort(i, "DrumFoundry input");
  inputs_.push_back(std::move(input));
}
MidiInputs::~MidiInputs() {
  for (auto &input : inputs_) {
    input->cancelCallback();
    input->closePort();
  }
}
} // namespace drumfoundry::standalone
