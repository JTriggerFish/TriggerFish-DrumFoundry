#include "eq_plot.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
EqPlot::EqPlot(editing::Document &d, const LiveSpectrum *s)
    : document_(d), spectrum_(s) {
  help =
      "Final EQ: drag the outer handles horizontally for high/low-pass cuts; "
      "drag the colour handle for frequency and gain; scroll over it for Q "
      "(Shift for fine adjustment). Double-click "
      "a handle to reset. Click a readout to type a precise value. "
      "Background: "
      "actual live output, fixed 0 to -96 dBFS/bin from top to bottom; curve "
      "grid: EQ gain in dB. Handles remain editable while bypassed; enable "
      "the EQ to hear those edits. Bypass never changes automatically.";
  addChild(&enabled_);
  enabled_.help = ParameterHelp("output_eq_enabled");
  enabled_.onToggle() = [this](auto *, bool) {
    document_.Set("output_eq_enabled",
                  document_.Value("output_eq_enabled") < .5);
    SyncReadouts();
    redraw();
    if (committed)
      committed();
  };
  for (unsigned i = 0; i < values_.size(); ++i) {
    addChild(&values_[i]);
    values_[i].onToggle() = [this, i](auto *, bool) { EditValue(i); };
  }
  addChild(&entry_, false);
  entry_.setMultiLine(false);
  entry_.onEnterKey() = [this] {
    SubmitValue(editing_, entry_.text().toUtf8());
  };
  entry_.onEscapeKey() = [this] { entry_.setVisible(false); };
  SyncReadouts();
}
float EqPlot::X(double f) const {
  return 30 +
         std::max(1.f, width() - 38) *
             float(std::log(std::clamp(f, 5., 22000.) / 5) / std::log(4400.));
}
float EqPlot::Y(double db) const {
  return 32 + std::max(1.f, height() - 137) *
                  float((24 - std::clamp(db, -36., 24.)) / 60);
}
double EqPlot::Frequency(float x) const {
  return 5 * std::pow(4400., (x - 30) / std::max(1.f, width() - 38));
}
double EqPlot::Gain(float y) const {
  return 24 - 60 * (y - 32) / std::max(1.f, height() - 137);
}
void EqPlot::draw(visage::Canvas &c) {
  const visage::theme::ColorId EqColours[]{colours::EqLow, colours::EqColour,
                                           colours::EqHigh};
  const bool enabled = document_.Value("output_eq_enabled") >= .5;
  const double rate = spectrum_ && spectrum_->Rate() ? spectrum_->Rate()
                      : previewRate                  ? previewRate()
                                                     : 48000;
  const auto p = editing::OutputEqSettings(document_);
  DrawBackground(c);
  SyncReadouts();
  if (spectrum_)
    spectrum_->DrawTrace(c, 30, 32, width() - 38, height() - 137,
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
