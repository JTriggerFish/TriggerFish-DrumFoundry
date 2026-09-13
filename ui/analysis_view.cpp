#include "analysis_view.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
void AnalysisView::Set(std::shared_ptr<const analysis::Result> result) {
  previousResult_.reset();
  writtenSeconds_ = -1;
  result_ = std::move(result);
  frequencyHigh = std::min(frequencyHigh, result_->model.sampleRate * .5);
  if (frequencyLow >= frequencyHigh)
    frequencyLow = 20;
  Refresh();
}
void AnalysisView::SetProgress(std::shared_ptr<const analysis::Result> result,
                               double seconds) {
  const bool sameContext =
      result_ && result_->referenceHash == result->referenceHash &&
      result_->referenceSpectrum.sampleRate ==
          result->referenceSpectrum.sampleRate &&
      result_->referenceSpectrum.size == result->referenceSpectrum.size &&
      result_->referenceSpectrum.hop == result->referenceSpectrum.hop &&
      result_->model.sampleRate == result->model.sampleRate;
  const double begin = result_ == result ? std::max(0., writtenSeconds_) : 0;
  if (result_ != result)
    previousResult_ = FreezeDisplayed();
  result_ = std::move(result);
  writtenSeconds_ = seconds;
  frequencyHigh = std::min(frequencyHigh, result_->model.sampleRate * .5);
  if (frequencyLow >= frequencyHigh)
    frequencyLow = 20;
  if (sameContext)
    RefreshRegion(begin, seconds);
  else
    Refresh();
}
void AnalysisView::RefreshRegion(double begin, double end) {
  partial_ = true;
  dirtyBegin_ = begin;
  dirtyEnd_ = end;
  Refresh();
  partial_ = false;
}
const analysis::Result &AnalysisView::ModelAt(double time) const {
  return previousResult_ && writtenSeconds_ >= 0 && time >= writtenSeconds_
             ? *previousResult_
             : *result_;
}
void AnalysisView::ResetZoom() {
  pan = 0;
  frequencyLow = 20;
  frequencyHigh = 20000;
  if (writtenSeconds_ >= 0)
    span = renderDuration;
  else if (result_)
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
      result_ ? std::min(frequencyHigh, result_->model.sampleRate * .5)
              : frequencyHigh;
  return {reference, time, maximum * std::pow(frequencyLow / maximum, v)};
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
      result_->referenceSpectrum.frames &&
              result_->referenceSpectrum.maximumDb > -180
          ? result_->referenceSpectrum.maximumDb + float(referenceGainDb)
          : 0;
  const double floor = ceiling - rangeDb;
  std::vector<Coordinate> horizontal;
  horizontal.reserve(columns);
  for (int x = 0; x < columns; ++x)
    horizontal.push_back(At(42 + (width() - 54) * (x + .5f) / columns, 62));
  for (int y = 0; y < rows; ++y) {
    const auto vertical = At(42, 62 + (height() - 91) * (y + .5f) / rows);
    const double verticalShare = comparison == Comparison::Stacked
                                     ? (vertical.reference ? split : 1 - split)
                                     : 1;
    const double halfBand = std::pow(
        std::min(frequencyHigh, result_->model.sampleRate * .5) / frequencyLow,
        .5 / (rows * verticalShare));
    const double frequencyLo = vertical.frequency / halfBand,
                 frequencyHi = vertical.frequency * halfBand;
    for (int x = 0; x < columns; ++x) {
      auto p = horizontal[x];
      p.frequency = vertical.frequency;
      if (comparison == Comparison::Stacked)
        p.reference = vertical.reference;
      const bool splitTime = comparison == Comparison::Mirror ||
                             comparison == Comparison::SideBySide;
      const double halfTime =
          .5 * span /
          (columns * (splitTime ? (p.reference ? split : 1 - split) : 1));
      if (partial_ &&
          (p.reference || p.time + modelOffset + halfTime < dirtyBegin_ ||
           p.time + modelOffset - halfTime > dirtyEnd_))
        continue;
      const double ref =
          result_->referenceSpectrum.Peak(p.time + referenceOffset - halfTime,
                                          p.time + referenceOffset + halfTime,
                                          frequencyLo, frequencyHi) +
          referenceGainDb;
      const double model =
          ModelAt(p.time + modelOffset)
              .modelSpectrum.Peak(p.time + modelOffset - halfTime,
                                  p.time + modelOffset + halfTime, frequencyLo,
                                  frequencyHi);
      const double value =
          comparison == Comparison::Difference
              ? .5 + .5 * (std::max(model, floor) - std::max(ref, floor)) /
                         differenceDb
              : ((p.reference ? ref : model) - floor) / rangeDb;
      heatmap_.set(x, y, float(std::clamp(value, 0., 1.)));
    }
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
  Readout(c);
  WriteEdge(c);
  if (comparison == Comparison::Mirror ||
      comparison == Comparison::SideBySide) {
    c.setColor(0xff8e9ead);
    c.fill(42 + float(split) * (width() - 54), 62, 1, height() - 91);
  } else if (comparison == Comparison::Stacked) {
    c.setColor(0xff8e9ead);
    c.fill(42, 62 + float(split) * (height() - 91), width() - 54, 1);
  }
}
void AnalysisView::WriteEdge(visage::Canvas &c) {
  if (writtenSeconds_ < 0 || comparison == Comparison::Reference)
    return;
  double x = (writtenSeconds_ - modelOffset - pan) / span;
  if (x < 0 || x > 1)
    return;
  if (comparison == Comparison::Mirror || comparison == Comparison::SideBySide)
    x = split + (1 - split) * x;
  const float top = comparison == Comparison::Stacked
                        ? 62 + float(split) * (height() - 91)
                        : 62;
  c.setColor(0xffa6adb5);
  c.fill(42 + float(x) * (width() - 54), top, 2, height() - 29 - top);
}
} // namespace drumfoundry::ui
