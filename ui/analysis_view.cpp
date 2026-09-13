#include "analysis_view.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
void AnalysisView::Set(std::shared_ptr<const analysis::Result> result) {
  result_ = std::move(result);
  Refresh();
}
void AnalysisView::ResetZoom() {
  pan = 0;
  if (result_)
    span = double(result_->model.samples.size()) / result_->model.sampleRate;
  Refresh();
}
AnalysisView::Coordinate AnalysisView::At(float x, float y) const {
  const double u =
      std::clamp(double((x - 42) / std::max(1.f, width() - 54)), 0., 1.);
  double v =
      std::clamp(double((y - 62) / std::max(1.f, height() - 91)), 0., 1.);
  bool reference = comparison == Comparison::Reference;
  double time = pan + span * u;
  if (comparison == Comparison::Mirror ||
      comparison == Comparison::SideBySide) {
    reference = u < split;
    const double local = reference ? u / split : (u - split) / (1 - split);
    time =
        pan + span * (reference && comparison == Comparison::Mirror ? 1 - local
                                                                    : local);
  } else if (comparison == Comparison::Stacked) {
    reference = v < split;
    v = reference ? v / split : (v - split) / (1 - split);
  }
  const double maximum =
      result_ ? std::min(20000., result_->model.sampleRate * .5) : 20000;
  return {reference, time, maximum * std::pow(20 / maximum, v)};
}
void AnalysisView::Refresh() {
  if (!result_ || width() < 55 || height() < 92) {
    redraw();
    return;
  }
  const int columns = std::clamp(int(width() - 54), 64, 2048),
            rows = std::clamp(int(height() - 91), 64, 768);
  heatmap_.setDimensions(columns, rows);
  // One reference-anchored ceiling for BOTH sides. Never adapt to model edits.
  const float ceiling =
      result_->referenceSpectrum.frames
          ? result_->referenceSpectrum.maximumDb + float(referenceGainDb)
          : 0;
  const double floor = ceiling - rangeDb;
  for (int y = 0; y < rows; ++y)
    for (int x = 0; x < columns; ++x) {
      const auto p = At(42 + (width() - 54) * x / std::max(1, columns - 1),
                        62 + (height() - 91) * y / std::max(1, rows - 1));
      const double ref =
          result_->referenceSpectrum.At(p.time + referenceOffset, p.frequency) +
          referenceGainDb;
      const double model =
          result_->modelSpectrum.At(p.time + modelOffset, p.frequency);
      const double value =
          comparison == Comparison::Difference
              ? .5 + .5 * (std::max(model, floor) - std::max(ref, floor)) /
                         differenceDb
              : ((p.reference ? ref : model) - floor) / rangeDb;
      heatmap_.set(x, y, float(std::clamp(value, 0., 1.)));
    }
  redraw();
}
void AnalysisView::draw(visage::Canvas &c) {
  c.setColor(0xff0b1016);
  c.fill(0, 0, width(), height());
  if (!result_) {
    Label(c, "Preparing native render…", 8, 8, width() - 16, 24);
    return;
  }
  if (comparison == Comparison::Difference)
    c.setColor(visage::Brush::horizontal(
        visage::Gradient(0xff65dce8, 0xff000000, 0xffffb458)));
  else
    c.setColor(visage::Brush::horizontal(visage::Gradient::kMagma));
  c.heatMap(heatmap_, 42, 62, width() - 54, height() - 91);
  Waveform(c);
  Axes(c);
  if (comparison == Comparison::Mirror ||
      comparison == Comparison::SideBySide) {
    c.setColor(0xff8e9ead);
    c.fill(42 + float(split) * (width() - 54), 62, 1, height() - 91);
  } else if (comparison == Comparison::Stacked) {
    c.setColor(0xff8e9ead);
    c.fill(42, 62 + float(split) * (height() - 91), width() - 54, 1);
  }
}
} // namespace drumfoundry::ui
