#pragma once
#include "controls.hpp"
#include "editing/output_eq.hpp"
#include "live_spectrum.hpp"
namespace drumfoundry::ui {
// Graph handles and editable readouts share the document's EQ parameters.
class EqPlot : public visage::Frame, public HelpText {
public:
  EqPlot(editing::Document &, const LiveSpectrum *);
  void draw(visage::Canvas &) override;
  void resized() override;
  bool SubmitValue(unsigned index, const std::string &text);
  void mouseDown(const visage::MouseEvent &) override;
  void mouseDrag(const visage::MouseEvent &) override;
  void mouseUp(const visage::MouseEvent &) override;
  void mouseMove(const visage::MouseEvent &) override;
  void mouseExit(const visage::MouseEvent &) override;
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
  void DrawBackground(visage::Canvas &);
  void DrawGrid(visage::Canvas &);
  void DrawHandles(visage::Canvas &);
  void EditValue(unsigned);
  void SyncReadouts();
  HelpButton enabled_;
  std::array<HelpButton, 4> values_;
  visage::TextEditor entry_;
  unsigned editing_{};
  std::array<double, 5> displayed_{};
  bool readoutsReady_{};
  editing::Document &document_;
  const LiveSpectrum *spectrum_;
  int drag_{-1};
  int hover_{-1};
};
} // namespace drumfoundry::ui
