#include "decay_editor.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace drumfoundry::ui {
using namespace editing;
DecayEditor::DecayEditor(Document &d) : document_(d) {
  addChild(&seconds_);
  addChild(&frequency_);
  addChild(&remove_);
  seconds_.position = DecayPosition;
  seconds_.valueAt = DecaySeconds;
  for (auto *slider : {&seconds_, &frequency_}) {
    slider->changed = [this](double) {
      try {
        SetDecay(document_, selected_, frequency_.Value(), seconds_.Value());
        Sync();
        if (changed)
          changed();
      } catch (const std::exception &e) {
        if (error)
          error(e.what());
      }
    };
    slider->committed = [this] {
      if (committed)
        committed();
    };
  }
  remove_.onToggle() = [this](auto *, bool) { Remove(); };
  Sync();
}
void DecayEditor::Sync() {
  for (auto p : DecayKnots(document_))
    if (p.slot == selected_) {
      seconds_.Set(p.seconds);
      seconds_.SetDefault(
          document_.Description("body_decay_seconds_" + std::to_string(p.slot))
              .initial);
      const double low = p.slot == 7 ? 15000 : 40;
      frequency_.SetRange(low, 20000);
      frequency_.position = [low](double f) {
        return (Erb(f) - Erb(low)) / (Erb(20000) - Erb(low));
      };
      frequency_.valueAt = [low](double v) {
        return InverseErb(Erb(low) + v * (Erb(20000) - Erb(low)));
      };
      frequency_.Set(p.frequency);
      frequency_.SetDefault(p.slot == 0
                                ? 40
                                : document_
                                      .Description("body_decay_frequency_" +
                                                   std::to_string(p.slot))
                                      .initial);
      frequency_.setIgnoresMouseEvents(p.slot == 0, false);
      frequency_.setAlphaTransparency(p.slot == 0 ? .5f : 1.f);
      frequency_.help =
          "Move the upper endpoint from 15 to 20 kHz to shape the very top "
          "end. Beyond the endpoint, decay stays constant.";
      remove_.setActive(!p.boundary);
      remove_.setAlphaTransparency(p.boundary ? .4f : 1.f);
    }
  redraw();
}
void DecayEditor::Remove() {
  try {
    DeleteDecay(document_, selected_);
    selected_ = 0;
    Sync();
    if (committed)
      committed();
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void DecayEditor::resized() {
  frequency_.setBounds(0, height() - 130, width(), 44);
  seconds_.setBounds(0, height() - 82, width(), 44);
  remove_.setBounds(0, height() - 30, width(), 28);
}
float DecayEditor::X(double f) const {
  return 36 + float((Erb(f) - Erb(40)) / (Erb(20000) - Erb(40))) *
                  std::max(1.f, width() - 50);
}
float DecayEditor::Y(double s) const {
  return 22 + float(1 - DecayPosition(s)) * std::max(1.f, height() - 190);
}
double DecayEditor::Frequency(float x) const {
  return InverseErb(
      Erb(40) +
      std::clamp(double((x - 36) / std::max(1.f, width() - 50)), 0., 1.) *
          (Erb(20000) - Erb(40)));
}
double DecayEditor::Seconds(float y) const {
  return DecaySeconds(1 - (y - 22) / std::max(1.f, height() - 190));
}
void DecayEditor::draw(visage::Canvas &c) {
  const auto points = DecayKnots(document_);
  Label(c,
        std::to_string(points.size()) +
            "/8 knots  ·  drag middle to move all",
        0, 0, width(), 18, colours::Muted);
  for (double f : {40., 1000., 20000.}) {
    c.setColor(colours::Grid);
    c.fill(X(f), 22, 1, height() - 190);
    Label(c,
          f == 40     ? "40 Hz"
          : f == 1000 ? "1k"
                      : "20k",
          X(f) - 18, height() - 163, 42, 18);
  }
  for (double s : {.1, 1., 5., 15., 30.}) {
    c.setColor(colours::Grid);
    c.fill(36, Y(s), width() - 50, 1);
    char label[16];
    std::snprintf(label, sizeof(label), "%.2g", s);
    Label(c, label, 0, Y(s) - 9, 32, 18);
  }
  visage::Path curve;
  for (int i = 0; i <= 100; ++i) {
    const float x = 36 + (width() - 50) * i / 100;
    const float y = Y(DecayAt(document_, Frequency(x)));
    if (!i)
      curve.moveTo(x, y);
    else
      curve.lineTo(x, y);
  }
  c.setColor(colours::Accent);
  c.fill(curve.stroke(1.5f));
  for (auto p : points) {
    c.setColor(p.slot == selected_ ? colours::Secondary : colours::Accent);
    if (p.boundary)
      c.fill(X(p.frequency) - 4, Y(p.seconds) - 4, 8, 8);
    else
      c.circle(X(p.frequency) - 5, Y(p.seconds) - 5, 10);
  }
  const float mx = (36 + width() - 14) / 2,
              my = Y(DecayAt(document_, Frequency(mx)));
  visage::Path diamond;
  diamond.moveTo(mx, my - 7);
  diamond.lineTo(mx + 7, my);
  diamond.lineTo(mx, my + 7);
  diamond.lineTo(mx - 7, my);
  diamond.close();
  c.setColor(colours::Heading);
  c.fill(diamond.stroke(1.5f));
}
} // namespace drumfoundry::ui
