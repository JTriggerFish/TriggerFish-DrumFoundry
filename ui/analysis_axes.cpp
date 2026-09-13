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
  if (comparison == Comparison::Stacked) {
    grid(62, float(split) * (height() - 91));
    grid(62 + float(split) * (height() - 91),
         float(1 - split) * (height() - 91));
  } else
    grid(62, height() - 91);
  for (int i = 0; i <= 6; ++i) {
    const float x = 42 + (width() - 54) * i / 6;
    char text[32];
    std::snprintf(text, sizeof(text), "%.2f s", At(x, 64).time);
    Label(c, text, std::clamp(x - 20, 0.f, width() - 65), height() - 26, 65,
          20);
  }
  std::string legend =
      comparison == Comparison::Difference
          ? "Cyan: less model · black: equal · amber: more model"
          : "Reference  |  TriggerFish — same dBFS/bin colour scale";
  if (!hover_)
    Label(c, legend, 48, 42, width() - 60, 18, 0xffe8b755);
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
  Label(c, text, 48, 42, width() - 60, 18, 0xffe8b755);
}
void AnalysisView::Waveform(visage::Canvas &c) {
  const double gain = std::pow(10., referenceGainDb / 20);
  double peak = .001;
  for (float v : result_->reference.samples)
    peak = std::max(peak, std::abs(v * gain));
  if (result_->reference.samples.empty())
    peak = 1;
  const int pixels = std::max(1, int(width() - 54));
  for (int source = 0; source < 2; ++source) {
    const auto &audio = source ? result_->model : result_->reference;
    const double offset = source ? modelOffset : referenceOffset;
    const float centre = source ? 31.f : 12.f;
    c.setColor(source ? 0xff8bbbeb : 0xffd6b25c);
    if (!audio.sampleRate)
      continue;
    for (int x = 0; x < pixels; ++x) {
      const auto &segment =
          source ? ModelAt(pan + offset + span * x / pixels).model : audio;
      const auto first =
          int64_t((pan + offset + span * x / pixels) * segment.sampleRate);
      const auto last = int64_t((pan + offset + span * (x + 1) / pixels) *
                                segment.sampleRate);
      float low = 0, high = 0;
      for (auto i = std::max<int64_t>(0, first);
           i <
           std::min<int64_t>(segment.samples.size(), std::max(first + 1, last));
           ++i) {
        const float v =
            segment.samples[std::size_t(i)] * float(source ? 1 : gain);
        low = std::min(low, v);
        high = std::max(high, v);
      }
      c.fill(42 + x, centre - float(std::clamp(high / peak, -1., 1.)) * 9, 1,
             std::max(.6f, float(std::clamp((high - low) / peak, 0., 2.)) * 9));
    }
  }
  char scale[28];
  std::snprintf(scale, sizeof(scale), "±%.2g", peak);
  Label(c, scale, 0, 0, 42, 38);
}
} // namespace drumfoundry::ui
