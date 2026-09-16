#pragma once
#include "editing/document.hpp"
namespace drumfoundry::ui {
inline editing::Json SoundIdentity(const editing::Json &document) {
  auto patch = document.at("instrument");
  for (auto &node : patch.at("nodes"))
    node.erase("editor");
  return {{"instrument", patch},
          {"event", document.at("controls").at("event")}};
}
// A brief pause in a drag can render a preview before mouse-up. This never
// prepares/replaces the live voice; live-safe edits use the parameter queue.
class PreviewTracker {
public:
  void Reset(const editing::Json &document) {
    observed_ = SoundIdentity(document);
    ticks_ = 0;
  }
  bool Advance(const editing::Json &document) {
    auto key = SoundIdentity(document);
    if (key != observed_) {
      observed_ = std::move(key);
      ticks_ = 4;
    } else if (ticks_ && --ticks_ == 0)
      return true;
    return false;
  }

private:
  editing::Json observed_;
  unsigned ticks_{};
};
} // namespace drumfoundry::ui
