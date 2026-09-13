#pragma once
#include <stdexcept>
#include <string>
#include <vector>

namespace drumfoundry::standalone {
// GUI names select exactly one port. Console abbreviations must be unambiguous.
inline std::vector<unsigned>
SelectMidiPorts(const std::vector<std::string> &names,
                const std::string &selection, bool exact = false) {
  std::vector<unsigned> selected;
  if (selection == "none")
    return selected;
  for (unsigned i = 0; i < names.size(); ++i)
    if (selection == "all" || names[i] == selection)
      selected.push_back(i);
  if (selection == "all")
    return selected;
  if (selected.empty() && !exact)
    for (unsigned i = 0; i < names.size(); ++i)
      if (names[i].find(selection) != std::string::npos)
        selected.push_back(i);
  if (selection.empty() || selected.size() != 1)
    throw std::runtime_error(
        selected.empty()
            ? "No MIDI input matching: " + selection
            : "Ambiguous MIDI input; choose its complete name: " + selection);
  return selected;
}
// An unavailable port must not discard other working inputs in 'all' mode.
template <typename Open>
std::string OpenMidiPorts(const std::vector<std::string> &names,
                          const std::string &selection, Open open,
                          bool exact = false) {
  std::string warning;
  for (auto index : SelectMidiPorts(names, selection, exact)) {
    try {
      open(index);
    } catch (const std::exception &error) {
      if (!warning.empty())
        warning += "\n";
      warning += "MIDI '" + names[index] + "': " + error.what();
    }
  }
  return warning;
}
} // namespace drumfoundry::standalone
