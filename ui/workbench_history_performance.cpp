#include "workbench.hpp"
#include <exception>

namespace drumfoundry::ui {
// Use requested values, not the audio thread's delayed readback. A complete
// slider gesture is recorded once, even when no audio callback has run yet.
void Workbench::EditPerformance(unsigned id, double value) {
  const auto key = id ? std::to_string(id) : "velocity";
  try {
    const double before =
        id ? bridge_.value(id) : (bridge_.velocity ? bridge_.velocity() : .8);
    if (id)
      bridge_.change(id, value);
    else if (bridge_.setVelocity)
      bridge_.setVelocity(value);
    if (!performanceBefore_.contains("parameters"))
      performanceBefore_ = {{"parameters", editing::Json::object()}};
    if (!performanceBefore_["parameters"].contains(key))
      performanceBefore_["parameters"][key] = before;
    performanceAfter_["parameters"][key] = value;
  } catch (const std::exception &e) {
    Error(e.what());
  }
}
void Workbench::CommitPerformance() {
  if (performanceBefore_.is_null())
    return;
  bridge_.history->Record(performanceBefore_, performanceAfter_);
  performanceBefore_ = performanceAfter_ = nullptr;
  UpdateHistoryButtons();
}
} // namespace drumfoundry::ui
