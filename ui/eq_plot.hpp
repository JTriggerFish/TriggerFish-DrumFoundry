#pragma once
#include "controls.hpp"
#include "editing/output_eq.hpp"
#include "live_spectrum.hpp"
namespace drumfoundry::ui {
// Three handles edit the same four visible scalar controls; no extra sound
// state.
class EqPlot : public visage::Frame, public HelpText {
public:
  EqPlot(editing::Document &, const LiveSpectrum *);
  void draw(visage::Canvas &) override;
  void mouseDown(const visage::MouseEvent &) override;
  void mouseDrag(const visage::MouseEvent &) override;
  void mouseUp(const visage::MouseEvent &) override;
  std::function<void()> changed, committed;
  std::function<unsigned()> previewRate;
  std::function<void(const std::string &)> error;

private:
  float X(double) const;
  float Y(double) const;
  double Frequency(float) const;
  double Gain(float) const;
  int Hit(visage::Point) const;
  void ResetHandle(int);
  editing::Document &document_;
  const LiveSpectrum *spectrum_;
  int drag_{-1};
};
} // namespace drumfoundry::ui
