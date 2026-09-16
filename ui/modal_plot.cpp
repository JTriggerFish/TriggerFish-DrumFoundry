#include "modal_plot.hpp"
#include "editing/curves.hpp"
#include "tfdsp/percussion/turbulence_profile.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace drumfoundry::ui {
using namespace editing;
void ModalPlot::Load(Document &d) {
  setAcceptsKeystrokes(true);
  document_ = &d;
  SelectOnly(-1);
  dragging_ = marquee_ = false;
  Refresh();
}
void ModalPlot::Refresh() {
  if (document_)
    modes_ = Modes(*document_);
  selection_.resize(modes_.size(), false);
  for (unsigned i = 0; i < modes_.size(); ++i)
    if (modes_[i].level <= -72)
      selection_[i] = false;
  if (selected >= int(modes_.size()) ||
      (selected >= 0 && modes_[selected].level <= -72))
    selected = -1;
  if (selected < 0)
    for (unsigned i = 0; i < selection_.size(); ++i)
      if (selection_[i]) {
        selected = int(i);
        break;
      }
  redraw();
  if (changed)
    changed();
}
float ModalPlot::X(double f) const {
  return 42 +
         float(std::log(std::clamp(f, 20., 15000.) / 20) / std::log(750.)) *
             std::max(1.f, width() - 60);
}
float ModalPlot::Y(double level) const {
  return 14 + float((6 - level) / 78) * std::max(1.f, height() - 44);
}
double ModalPlot::Frequency(float x) const {
  return 20 * std::pow(750., std::clamp(
                                 double((x - 42) / std::max(1.f, width() - 60)),
                                 0., 1.));
}
double ModalPlot::Level(float y) const {
  return std::clamp(6 - 78. * (y - 14) / std::max(1.f, height() - 44), -72.,
                    6.);
}
double ModalPlot::Snap(double f) const {
  if (guide && snap)
    f = std::clamp(std::round(f / base), std::ceil(20 / base),
                   std::floor(15000 / base)) *
        base;
  return std::clamp(f, 20., 15000.);
}
double ModalPlot::Spread(const Mode &m) const {
  if (!document_ || ModePrefix(*document_) != "resolved_")
    return 0;
  return tfdsp::percussion::EvaluateTurbulence(
             float(m.frequency), float(document_->Value("field_turbulence")),
             float(document_->Value("field_turbulence_slope")), 1000,
             float(m.turbulence), true)
             .intensity *
         document_->Value("field_packet_spread");
}
double ModalPlot::PacketFrequency(const Mode &m, double offset) const {
  if (document_->Value("field_distribution") == 4)
    return std::clamp(m.frequency + offset * 24.7 * (1 + .00437 * m.frequency),
                      20., 15000.);
  return InverseErb(std::clamp(Erb(m.frequency) + offset, Erb(20), Erb(15000)));
}
void ModalPlot::draw(visage::Canvas &c) {
  c.setColor(colours::Plot);
  c.fill(0, 0, width(), height());
  for (double f :
       {20., 50., 100., 200., 500., 1000., 2000., 5000., 10000., 15000.}) {
    c.setColor(colours::Grid);
    c.fill(X(f), 14, 1, height() - 44);
    char label[24];
    std::snprintf(label, sizeof(label), f >= 1000 ? "%.2gk" : "%.0f",
                  f >= 1000 ? f / 1000 : f);
    Label(c, label, X(f) - 16, height() - 25, 36, 22);
  }
  for (double level : {6., 0., -24., -48., -72.}) {
    c.setColor(colours::Grid);
    c.fill(42, Y(level), width() - 60, 1);
    Label(c, level == -72 ? "off" : std::to_string(int(level)), 4,
          Y(level) - 10, 36, 20);
  }
  if (guide && base >= 8)
    for (double f = base; f <= 15000; f += base) {
      if (f < 20)
        continue;
      c.setColor(c.color(colours::Secondary).withMultipliedAlpha(.3f));
      c.fill(X(f), 14, 1, height() - 44);
    }
  const bool multiple = SelectionCount() > 1;
  for (unsigned i = 0; i < modes_.size(); ++i) {
    const auto &m = modes_[i];
    if (m.level <= -72 || m.frequency < 20)
      continue;
    const auto spread = Spread(m);
    if (spread > 0) {
      visage::Path packet;
      packet.moveTo(X(PacketFrequency(m, -3 * spread)), Y(-72));
      for (int j = -30; j <= 30; ++j) {
        const double offset = j / 10.;
        packet.lineTo(
            X(PacketFrequency(m, spread * offset)),
            Y(-72 + (m.level + 72) * std::exp(-.5 * offset * offset)));
      }
      packet.lineTo(X(PacketFrequency(m, 3 * spread)), Y(-72));
      packet.close();
      c.setColor(IsSelected(i)
                     ? c.color(colours::Secondary).withMultipliedAlpha(.25f)
                     : c.color(colours::Accent).withMultipliedAlpha(.16f));
      c.fill(packet);
    }
    c.setColor(IsSelected(i) ? colours::Secondary : colours::Accent);
    c.fill(X(m.frequency) - .75f, Y(m.level), 1.5f, Y(-72) - Y(m.level));
    c.circle(X(m.frequency) - 4, Y(m.level) - 4, 8);
    if (multiple && int(i) == selected) {
      c.setColor(colours::Text);
      c.circle(X(m.frequency) - 2, Y(m.level) - 2, 4);
    }
  }
  DrawSelection(c);
}
} // namespace drumfoundry::ui
