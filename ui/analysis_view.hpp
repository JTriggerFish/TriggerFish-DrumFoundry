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
  double rangeDb{80}, differenceDb{24}, referenceGainDb{}, referenceOffset{},
      modelOffset{};
  double span{8}, pan{}, split{.5};
  double frequencyLow{20}, frequencyHigh{20000};

private:
  struct Coordinate {
    bool reference;
    double time, frequency;
  };
  Coordinate At(float x, float y) const;
  void Waveform(visage::Canvas &);
  void Axes(visage::Canvas &);
  void Readout(visage::Canvas &);
  std::shared_ptr<const analysis::Result> result_;
  visage::HeatMapData heatmap_;
  bool dragging_{}, divider_{};
  visage::Point previous_;
  visage::Point pointer_;
  bool hover_{};
};
} // namespace drumfoundry::ui
