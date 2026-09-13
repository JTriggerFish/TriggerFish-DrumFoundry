#pragma once
#include "controls.hpp"
#include "workbench/analysis/worker.hpp"
namespace drumfoundry::ui {
enum class Comparison {
  Mirror,
  SideBySide,
  Stacked,
  Difference,
  Model,
  Reference
};
class AnalysisView : public visage::Frame {
public:
  void Set(std::shared_ptr<const analysis::Result>);
  void SetProgress(std::shared_ptr<const analysis::Result>, double seconds);
  void Refresh();
  void ResetZoom();
  void resized() override { Refresh(); }
  void draw(visage::Canvas &) override;
  bool mouseWheel(const visage::MouseEvent &) override;
  void mouseDown(const visage::MouseEvent &) override;
  void mouseDrag(const visage::MouseEvent &) override;
  void mouseUp(const visage::MouseEvent &) override;
  void mouseMove(const visage::MouseEvent &) override;
  void mouseExit(const visage::MouseEvent &) override {
    hover_ = false;
    redraw();
  }
  Comparison comparison{Comparison::Mirror};
  bool showReference{true};
  bool HasReference() const {
    return showReference && result_ && !result_->reference.samples.empty();
  }
  Comparison Mode() const {
    return HasReference() ? comparison : Comparison::Model;
  }
  double rangeDb{80}, differenceDb{24}, referenceGainDb{}, referenceOffset{},
      modelOffset{};
  double span{8}, pan{}, split{.5};
  double renderDuration{8};
  double frequencyLow{20}, frequencyHigh{20000};

private:
  static constexpr float PlotTop =
      120; // Two waveform lanes, time axis, legend.
  struct Coordinate {
    bool reference;
    double time, frequency;
  };
  Coordinate At(float x, float y) const;
  void Waveform(visage::Canvas &);
  void Axes(visage::Canvas &);
  void Readout(visage::Canvas &);
  void WriteEdge(visage::Canvas &);
  const analysis::Result &ModelAt(double time) const;
  std::shared_ptr<const analysis::Result> FreezeDisplayed() const;
  void RefreshRegion(double begin, double end);
  bool partial_{};
  double dirtyBegin_{}, dirtyEnd_{};
  std::shared_ptr<const analysis::Result> result_;
  std::shared_ptr<const analysis::Result> previousResult_;
  double writtenSeconds_{-1};
  visage::HeatMapData heatmap_;
  bool dragging_{}, divider_{}, dragReference_{}, dragWaveform_{};
  visage::Point previous_;
  visage::Point pointer_;
  bool hover_{};
};
} // namespace drumfoundry::ui
