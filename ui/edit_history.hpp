#pragma once
#include <nlohmann/json.hpp>
#include <vector>

namespace drumfoundry::ui {
// Main-thread, per-instance history. Store patches so undo leaves unrelated
// performance/viewport changes alone. Never serialize history into presets.
class EditHistory {
public:
  using Json = nlohmann::json;
  bool CanUndo() const { return cursor_ > 0; }
  bool CanRedo() const { return cursor_ < entries_.size(); }
  void Clear() {
    entries_.clear();
    cursor_ = 0;
  }
  void Record(const Json &before, const Json &after, bool merge = false) {
    if (before == after)
      return;
    if (merge && CanUndo() && !CanRedo() && entries_.back().after == before) {
      auto original = entries_.back().before;
      entries_.pop_back();
      --cursor_;
      Record(original, after);
      return;
    }
    entries_.resize(cursor_);
    entries_.push_back(
        {before, after, Json::diff(after, before), Json::diff(before, after)});
    if (entries_.size() > 128)
      entries_.erase(entries_.begin());
    cursor_ = entries_.size();
  }
  Json Target(const Json &current, bool redo) const {
    return current.patch(Patch(redo));
  }
  const Json &Patch(bool redo) const {
    return redo ? entries_.at(cursor_).forward
                : entries_.at(cursor_ - 1).reverse;
  }
  // Call only after the host successfully accepts Target().
  void Accept(bool redo) { redo ? ++cursor_ : --cursor_; }

private:
  struct Entry {
    Json before, after, reverse, forward;
  };
  std::vector<Entry> entries_;
  std::size_t cursor_{};
};
} // namespace drumfoundry::ui
