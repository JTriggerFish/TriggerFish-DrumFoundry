#include "analysis_view.hpp"
#include "time_axis.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace drumfoundry::ui {
void AnalysisView::Axes(visage::Canvas &c) {
  const double maximum =
      std::min(frequencyHigh, result_->model.sampleRate * .5);
  const auto grid = [&](float top, float h) {
    for (double f :
         {20., 50., 100., 200., 500., 1000., 2000., 5000., 10000.}) {
      if (f > maximum || f < frequencyLow)
        continue;
      const float y =
          top +
          float(std::log(maximum / f) / std::log(maximum / frequencyLow)) * h;
      c.setColor(c.color(colours::Grid).withMultipliedAlpha(.4f));
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
  const bool horizontal =
      Mode() == Comparison::Mirror || Mode() == Comparison::SideBySide;
  for (const auto &tick :
       TimeAxisTicks(42, width() - 54, paletteValue(TextScale), pan, span,
                     horizontal ? split : 0, Mode() == Comparison::Mirror)) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.2f s", tick.seconds);
    c.setColor(colours::Muted);
    c.fill(tick.x, height() - 29, 1, 3);
    Label(c, ElideText(FrameFont(*this), text, tick.labelWidth), tick.labelX,
          height() - 26, tick.labelWidth, 20);
  }
  if (!hover_) {
    const float y = PlotTop - 20;
    if (Mode() == Comparison::Difference)
      Label(c, "Cyan: less model · black: equal · amber: more model", 48, y,
            width() - 60, 18, colours::Muted);
    else if (HasReference()) {
      const float labelWidth = 82 * paletteValue(TextScale);
      Label(c, "Reference", 48, y, labelWidth, 18, colours::Reference);
      Label(c, "TriggerFish", 48 + labelWidth, y, labelWidth + 12, 18,
            colours::Synth);
      const float x = 64 + 2 * labelWidth;
      if (width() - x > 230 * paletteValue(TextScale))
        Label(c, "Same dBFS/bin colour scale", x, y, width() - x - 12, 18,
              colours::Muted);
    } else
      Label(c, "TriggerFish — synth only", 48, y, width() - 60, 18,
            colours::Synth);
  }
}
void AnalysisView::Readout(visage::Canvas &c) {
  if (!hover_)
    return;
  const auto p = At(pointer_.x, pointer_.y);
  const double ref =
      result_->referenceSpectrum.At(p.time + referenceOffset, p.frequency) +
      referenceGainDb;
  const double model =
      ModelAt(p.time + modelOffset)
          .modelSpectrum.At(p.time + modelOffset, p.frequency);
  char text[160];
  std::snprintf(text, sizeof(text),
                "%.3f s · %.1f Hz · Ref %.1f / Model %.1f dBFS · Δ %+.1f dB",
                p.time, p.frequency, ref, model, model - ref);
  if (!HasReference())
    std::snprintf(text, sizeof(text), "%.3f s · %.1f Hz · Model %.1f dBFS",
                  p.time, p.frequency, model);
  Label(c, text, 48, PlotTop - 20, width() - 60, 18, colours::Heading);
}
} // namespace drumfoundry::ui
