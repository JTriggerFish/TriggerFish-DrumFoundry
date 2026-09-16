#include "eq_plot.hpp"
#include <algorithm>
#include <cstdio>

namespace drumfoundry::ui {
void EqPlot::DrawBackground(visage::Canvas &c) {
  c.setColor(colours::Plot);
  c.roundedRectangle(0, 0, width(), height(), 5);
  c.setColor(colours::Border);
  c.roundedRectangleBorder(.5f, .5f, width() - 1, height() - 1, 5, 1);
  std::string label = "FINAL EQ";
  const int selected = drag_ >= 0 ? drag_ : hover_;
  if (selected >= 0) {
    const auto p = editing::OutputEqSettings(document_);
    const double hz[]{p.lowCutHz, p.colourFrequencyHz, p.highCutHz};
    const char *names[]{"High-pass", "Colour", "Low-pass"};
    char value[80];
    if (selected == 1)
      std::snprintf(value, sizeof(value), "Colour %.0f Hz / %+.1f dB", hz[1],
                    p.colourGainDb);
    else
      std::snprintf(value, sizeof(value), "%s %.0f Hz", names[selected],
                    hz[selected]);
    label = value;
  }
  const float badge = enabled_.width();
  const float space = std::max(0.f, width() - badge - 24);
  Label(c, ElideText(FrameFont(*this), label, space), 8, 1, space, 22);
}
void EqPlot::DrawGrid(visage::Canvas &c) {
  for (double db : {-24., -12., 0., 12.}) {
    c.setColor(c.color(db == 0 ? colours::Border : colours::Grid)
                   .withMultipliedAlpha(db == 0 ? 1.f : .55f));
    c.fill(30, Y(db), width() - 38, 1);
    Label(c, std::to_string(int(db)), 3, Y(db) - 8, 25, 16, colours::Muted);
  }
  for (double f : {100., 1000., 10000.}) {
    c.setColor(c.color(colours::Grid).withMultipliedAlpha(.55f));
    c.fill(X(f), 32, 1, height() - 137);
    Label(c,
          f == 100    ? "100"
          : f == 1000 ? "1k"
                      : "10k",
          X(f) - 10, height() - 104, 32, 18, colours::Muted);
  }
}
void EqPlot::DrawHandles(visage::Canvas &c) {
  const auto p = editing::OutputEqSettings(document_);
  const double frequencies[]{p.lowCutHz, p.colourFrequencyHz, p.highCutHz};
  const visage::theme::ColorId handleColours[]{
      colours::EqLow, colours::EqColour, colours::EqHigh};
  for (int i = 0; i < 3; ++i) {
    const float x = X(frequencies[i]), y = Y(i == 1 ? p.colourGainDb : 0);
    const bool active = i == drag_ || i == hover_;
    c.setColor(active ? colours::Text : colours::Plot);
    c.circle(x - 9, y - 9, 18);
    c.setColor(colours::Plot);
    c.circle(x - 7.5f, y - 7.5f, 15);
    c.setColor(handleColours[i]);
    c.circle(x - 6, y - 6, 12);
  }
}
} // namespace drumfoundry::ui
