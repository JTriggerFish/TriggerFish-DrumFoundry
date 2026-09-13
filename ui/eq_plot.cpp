#include "eq_plot.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
namespace {
constexpr unsigned colours[]{0xffdcb66c, 0xffc78ac0, 0xff74b8d6};
}
EqPlot::EqPlot(editing::Document &d, const LiveSpectrum *s)
    : document_(d), spectrum_(s) {
  help =
      "Final EQ: drag gold/blue handles horizontally for high/low-pass cuts; "
      "drag pink in two dimensions for colour frequency and gain. Double-click "
      "a handle to reset. Values below are the same controls. Background: "
      "actual live output, fixed 0 to -96 dBFS/bin from top to bottom; curve "
      "grid: EQ gain in dB. Bypass disables the response, not the spectrum.";
}
float EqPlot::X(double f) const {
  return 30 +
         std::max(1.f, width() - 38) *
             float(std::log(std::clamp(f, 20., 20000.) / 20) / std::log(1000.));
}
float EqPlot::Y(double db) const {
  return 26 + std::max(1.f, height() - 50) *
                  float((18 - std::clamp(db, -36., 18.)) / 54);
}
double EqPlot::Frequency(float x) const {
  return 20 * std::pow(1000., (x - 30) / std::max(1.f, width() - 38));
}
double EqPlot::Gain(float y) const {
  return 18 - 54 * (y - 26) / std::max(1.f, height() - 50);
}
void EqPlot::draw(visage::Canvas &c) {
  const bool enabled = document_.Value("output_eq_enabled") >= .5;
  const double rate = spectrum_ && spectrum_->Rate() ? spectrum_->Rate()
                      : previewRate                  ? previewRate()
                                                     : 48000;
  const auto p = editing::OutputEqSettings(document_);
  Label(c,
        enabled ? "FINAL EQ  /  live output behind"
                : "EQ BYPASSED  /  live output behind",
        0, 0, width(), 20, 0xff8799ae);
  if (spectrum_)
    spectrum_->DrawTrace(c, 30, 26, width() - 38, height() - 50, 0xff233d50,
                         true);
  for (double db : {-24., -12., 0., 12.}) {
    c.setColor(db == 0 ? 0xff536778 : 0xff293440);
    c.fill(30, Y(db), width() - 38, 1);
    Label(c, std::to_string(int(db)), 0, Y(db) - 8, 28, 16);
  }
  for (double f : {100., 1000., 10000.}) {
    c.setColor(0xff293440);
    c.fill(X(f), 26, 1, height() - 50);
    Label(c,
          f == 100    ? "100"
          : f == 1000 ? "1k"
                      : "10k",
          X(f) - 10, height() - 21, 32, 18);
  }
  std::array<visage::Path, 4> paths;
  const editing::OutputEqResponse response(p, rate);
  const double maximum = std::min(20000., .499 * rate);
  for (int i = 0; i < 160; ++i) {
    const double f = 20 * std::pow(maximum / 20, i / 159.);
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
    c.setColor(i == 3 ? 0xffd5e3ef
                      : ((enabled ? 0x99000000 : 0x33000000) |
                         (colours[i] & 0xffffff)));
    c.fill(paths[i].stroke(i == 3 ? 1.5f : 1.f));
  }
  const double frequencies[]{p.lowCutHz, p.colourFrequencyHz, p.highCutHz};
  for (int i = 0; i < 3; ++i) {
    c.setColor((enabled ? 0xff000000 : 0x66000000) | (colours[i] & 0xffffff));
    c.circle(X(frequencies[i]) - 5, Y(i == 1 ? p.colourGainDb : 0) - 5, 10);
  }
}
} // namespace drumfoundry::ui
