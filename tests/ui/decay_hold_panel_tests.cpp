#include "ui/decay_hold_panel.hpp"
#include <stdexcept>
void DecayHoldPolicy(drumfoundry::editing::Document document) {
  using namespace drumfoundry;
  ui::DecayHoldPanel panel;
  const auto baseline = document.JsonValue();
  document.Set("bloom_rate", 1);
  const auto edited = document.JsonValue();
  panel.Edited(baseline, edited, 48000);
  if (!panel.NeedsPoll())
    throw std::runtime_error("Bloom edit did not arm hold decay");
  auto presentation = edited;
  presentation["controls"]["analysis"]["view"]["span"] = 3;
  presentation["instrument"]["nodes"][0]["editor"]["x"] = 123;
  panel.Poll(presentation);
  if (!panel.NeedsPoll())
    throw std::runtime_error("Presentation edit cancelled hold decay");
  auto performance = edited;
  performance["controls"]["event"]["strength"] = .123;
  panel.Poll(performance);
  if (panel.NeedsPoll())
    throw std::runtime_error("New strike did not cancel hold decay");
  panel.Edited(baseline, edited, 48000);
  panel.Cancel();
  if (panel.NeedsPoll())
    throw std::runtime_error("Hold-decay explicit cancellation failed");
}
