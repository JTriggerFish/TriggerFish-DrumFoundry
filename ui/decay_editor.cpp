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
  frequency_.position = [](double f) {
    return (Erb(f) - Erb(40)) / (Erb(15000) - Erb(40));
  };
  frequency_.valueAt = [](double p) {
    return InverseErb(Erb(40) + p * (Erb(15000) - Erb(40)));
  };
  for (auto *slider : {&seconds_, &frequency_}) {
    slider->changed = [this](double) {
      try {
        SetDecay(document_, selected_, frequency_.Value(), seconds_.Value());
        Sync();
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
      frequency_.Set(p.frequency);
      frequency_.setIgnoresMouseEvents(p.boundary, false);
      frequency_.setAlphaTransparency(p.boundary ? .5f : 1.f);
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
  return 36 + float((Erb(f) - Erb(40)) / (Erb(15000) - Erb(40))) *
                  std::max(1.f, width() - 50);
}
float DecayEditor::Y(double s) const {
  return 22 + float(1 - DecayPosition(s)) * std::max(1.f, height() - 190);
}
double DecayEditor::Frequency(float x) const {
  return InverseErb(
      Erb(40) +
      std::clamp(double((x - 36) / std::max(1.f, width() - 50)), 0., 1.) *
          (Erb(15000) - Erb(40)));
}
double DecayEditor::Seconds(float y) const {
  return DecaySeconds(1 - (y - 22) / std::max(1.f, height() - 190));
}
void DecayEditor::draw(visage::Canvas &c) {
  const auto points = DecayKnots(document_);
  Label(c,
        std::to_string(points.size()) + "/8 knots  ·  drag middle to move all",
        0, 0, width(), 18, 0xff8799ae);
  for (double f : {40., 1000., 15000.}) {
    c.setColor(0xff293440);
    c.fill(X(f), 22, 1, height() - 190);
    Label(c,
          f == 40     ? "40 Hz"
          : f == 1000 ? "1k"
                      : "15k",
          X(f) - 18, height() - 163, 42, 18);
  }
  for (double s : {.1, 1., 5., 15., 30.}) {
    c.setColor(0xff293440);
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
  c.setColor(0xff9fcaff);
  c.fill(curve.stroke(1.5f));
  for (auto p : points) {
    c.setColor(p.slot == selected_ ? 0xffe8b755 : 0xff9fcaff);
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
  c.setColor(0xffe8b755);
  c.fill(diamond.stroke(1.5f));
}
} // namespace drumfoundry::ui
