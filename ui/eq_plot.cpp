#include "eq_plot.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
EqPlot::EqPlot(editing::Document &d, const LiveSpectrum *s)
    : document_(d), spectrum_(s) {
  help =
      "Final EQ: drag the outer handles horizontally for high/low-pass cuts; "
      "drag the colour handle for frequency and gain. Double-click "
      "a handle to reset. Values below are the same controls. Background: "
      "actual live output, fixed 0 to -96 dBFS/bin from top to bottom; curve "
      "grid: EQ gain in dB. Handles remain editable while bypassed; enable "
      "the EQ below to hear those edits. Bypass never changes automatically.";
}
float EqPlot::X(double f) const {
  return 30 +
         std::max(1.f, width() - 38) *
             float(std::log(std::clamp(f, 5., 22000.) / 5) / std::log(4400.));
}
float EqPlot::Y(double db) const {
  return 26 + std::max(1.f, height() - 50) *
                  float((24 - std::clamp(db, -36., 24.)) / 60);
}
double EqPlot::Frequency(float x) const {
  return 5 * std::pow(4400., (x - 30) / std::max(1.f, width() - 38));
}
double EqPlot::Gain(float y) const {
  return 24 - 60 * (y - 26) / std::max(1.f, height() - 50);
}
void EqPlot::draw(visage::Canvas &c) {
  const visage::theme::ColorId EqColours[]{colours::EqLow, colours::EqColour,
                                           colours::EqHigh};
  const bool enabled = document_.Value("output_eq_enabled") >= .5;
  const double rate = spectrum_ && spectrum_->Rate() ? spectrum_->Rate()
                      : previewRate                  ? previewRate()
                                                     : 48000;
  const auto p = editing::OutputEqSettings(document_);
  DrawBackground(c, enabled);
  if (spectrum_)
    spectrum_->DrawTrace(c, 30, 26, width() - 38, height() - 50,
                         colours::Spectrum, true, .4f, 5, 22000);
  DrawGrid(c);
  std::array<visage::Path, 4> paths;
  const editing::OutputEqResponse response(p, rate);
  const double maximum = std::min(22000., .499 * rate);
  for (int i = 0; i < 160; ++i) {
    const double f = 5 * std::pow(maximum / 5, i / 159.);
    auto parts = response.At(f);
    const double total = enabled ? parts[0] + parts[1] + parts[2] : 0;
    for (unsigned stage = 0; stage < 4; ++stage) {
      const float y = Y(stage == 3 ? total : parts[stage]);
      if (!i)
        paths[stage].moveTo(X(f), y);
      else
        paths[stage].lineTo(X(f), y);
    }
  }
  for (unsigned i = 0; i < 4; ++i) {
    if (i == 3) {
      c.setColor(colours::Plot);
      c.fill(paths[i].stroke(4));
    }
    c.setColor(i == 3 ? c.color(colours::Text)
                      : c.color(EqColours[i])
                            .withMultipliedAlpha(enabled ? .9f : .55f));
    c.fill(paths[i].stroke(i == 3 ? 2.25f : 1.5f));
  }
  DrawHandles(c);
}
} // namespace drumfoundry::ui
