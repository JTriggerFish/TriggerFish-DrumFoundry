#include "workbench.hpp"
#include <algorithm>

namespace drumfoundry::ui {
editing::Json Workbench::MergedEdit(const editing::Json &current) {
  auto result = document_.JsonValue();
  if (displayedDocument_.is_null() || result.at("instrument").at("recipe") !=
                                          current.at("instrument").at("recipe"))
    return result;
  const auto &previous =
      gestureDocument_.is_null() ? displayedDocument_ : gestureDocument_;
  for (const auto &node : result.at("instrument").at("nodes"))
    for (const auto &old : previous.at("instrument").at("nodes"))
      if (node.at("id") == old.at("id"))
        for (const auto &[key, value] : node.at("parameters").items())
          if (old.at("parameters").contains(key) &&
              value != old.at("parameters").at(key) &&
              std::find(gestureKeys_.begin(), gestureKeys_.end(), key) ==
                  gestureKeys_.end())
            gestureKeys_.push_back(key);
  gestureDocument_ = result;
  // Only locally edited parameters override host automation since the last
  // refresh. A drag on the EQ must not rewind an automated decay or bloom knob.
  for (auto &node : result["instrument"]["nodes"])
    for (const auto &baseline :
         displayedDocument_.at("instrument").at("nodes")) {
      if (node.at("id") != baseline.at("id"))
        continue;
      for (const auto &live : current.at("instrument").at("nodes")) {
        if (node.at("id") != live.at("id"))
          continue;
        for (auto &[key, value] : node["parameters"].items())
          if (std::find(gestureKeys_.begin(), gestureKeys_.end(), key) ==
                  gestureKeys_.end() &&
              live.at("parameters").contains(key))
            value = live.at("parameters").at(key);
      }
    }
  return result;
}

void Workbench::PollAutomation() {
  if (!bridge_.automationRevision || !liveEditBefore_.is_null())
    return;
  const auto revision = bridge_.automationRevision();
  if (revision == automationRevision_)
    return;
  automationRevision_ = revision;
  const auto current = bridge_.document();
  if (current.at("instrument").at("recipe") != document_.Recipe())
    return;
  std::vector<std::pair<std::string, double>> changes;
  for (const auto &node : current.at("instrument").at("nodes"))
    for (auto &baseline : displayedDocument_["instrument"]["nodes"]) {
      if (node.at("id") != baseline.at("id"))
        continue;
      for (const auto &[key, value] : node.at("parameters").items()) {
        // Leave uncommitted structural drags alone while unrelated host
        // automation updates the rest of the editor.
        if (document_.Value(key) ==
            baseline.at("parameters").at(key).get<double>()) {
          if (document_.Value(key) != value.get<double>())
            changes.emplace_back(key, value.get<double>());
          baseline["parameters"][key] = value;
        }
      }
    }
  if (changes.empty())
    return;
  document_.SetMany(changes);
  if (!gestureDocument_.is_null())
    gestureDocument_ = document_.JsonValue();
  excitation_.SyncValues(document_);
  resonance_.SyncValues(document_);
}

editing::Json Workbench::LiveEditBaseline(const editing::Json &after) const {
  auto before = after;
  // History includes only this gesture, not DAW automation or incoming MIDI.
  for (auto &node : before["instrument"]["nodes"])
    for (const auto &original : liveEditBefore_.at("instrument").at("nodes")) {
      if (node.at("id") != original.at("id"))
        continue;
      for (auto &[key, value] : node["parameters"].items())
        if (std::find(gestureKeys_.begin(), gestureKeys_.end(), key) !=
            gestureKeys_.end())
          value = original.at("parameters").at(key);
    }
  return before;
}
} // namespace drumfoundry::ui
