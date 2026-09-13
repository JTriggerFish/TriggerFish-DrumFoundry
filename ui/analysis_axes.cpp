#include "analysis_view.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace drumfoundry::ui {
void AnalysisView::Axes(visage::Canvas &c) {
  const double maximum =
      std::min(frequencyHigh, result_->model.sampleRate * .5);
  const auto grid = [&](float top, float h) {
    for (double f : {20., 50., 100., 200., 500., 1000., 2000., 5000., 10000.}) {
      if (f > maximum || f < frequencyLow)
        continue;
      const float y =
          top +
          float(std::log(maximum / f) / std::log(maximum / frequencyLow)) * h;
      c.setColor(0x502d3b48);
      c.fill(42, y, width() - 54, 1);
      char text[20];
      std::snprintf(text, sizeof(text), f >= 1000 ? "%.0fk" : "%.0f",
                    f >= 1000 ? f / 1000 : f);
      Label(c, text, 2, y - 8, 37, 16);
    }
  };
  if (Mode() == Comparison::Stacked) {
    grid(PlotTop, float(split) * (height() - PlotTop - 29));
    grid(PlotTop + float(split) * (height() - PlotTop - 29),
         float(1 - split) * (height() - PlotTop - 29));
  } else
    grid(PlotTop, height() - PlotTop - 29);
  for (int i = 0; i <= 6; ++i) {
    const float x = 42 + (width() - 54) * i / 6;
    char text[32];
    std::snprintf(text, sizeof(text), "%.2f s", At(x, PlotTop + 2).time);
    Label(c, text, std::clamp(x - 20, 0.f, width() - 65), height() - 26, 65,
          20);
  }
  std::string legend =
      Mode() == Comparison::Difference
          ? "Cyan: less model · black: equal · amber: more model"
      : HasReference()
          ? "Reference  |  TriggerFish — same dBFS/bin colour scale"
          : "TriggerFish — synth only";
  if (!hover_)
    Label(c, legend, 48, PlotTop - 20, width() - 60, 18, 0xffe8b755);
}
void AnalysisView::Readout(visage::Canvas &c) {
  if (!hover_)
    return;
  const auto p = At(pointer_.x, pointer_.y);
  const double ref =
      result_->referenceSpectrum.At(p.time + referenceOffset, p.frequency) +
      referenceGainDb;
  const double model = ModelAt(p.time + modelOffset)
                           .modelSpectrum.At(p.time + modelOffset, p.frequency);
  char text[160];
  std::snprintf(text, sizeof(text),
                "%.3f s · %.1f Hz · Ref %.1f / Model %.1f dBFS · Δ %+.1f dB",
                p.time, p.frequency, ref, model, model - ref);
  if (!HasReference())
    std::snprintf(text, sizeof(text), "%.3f s · %.1f Hz · Model %.1f dBFS",
                  p.time, p.frequency, model);
  Label(c, text, 48, PlotTop - 20, width() - 60, 18, 0xffe8b755);
}
} // namespace drumfoundry::ui
