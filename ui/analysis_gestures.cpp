#include "analysis_view.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
bool AnalysisView::mouseWheel(const visage::MouseEvent &e) {
  if (e.isAltDown()) {
    const double centre = At(e.position.x, e.position.y).frequency;
    const double scale =
        std::clamp(std::exp(-.12 * e.precise_wheel_delta_y), .2, 5.);
    const double top =
        result_ ? std::min(20000., result_->model.sampleRate * .5) : 20000;
    const double lo =
        std::max(20., centre * std::pow(frequencyLow / centre, scale));
    const double hi =
        std::min(top, centre * std::pow(frequencyHigh / centre, scale));
    if (hi / lo > 1.02) {
      frequencyLow = lo;
      frequencyHigh = hi;
    }
  } else if (e.isCtrlDown() || e.isCmdDown()) {
    const double fraction = (At(e.position.x, e.position.y).time - pan) / span;
    const double next =
        std::clamp(span * std::exp(-.12 * e.precise_wheel_delta_y), .02, 60.);
    pan += (span - next) * fraction;
    span = next;
  } else
    pan -= span * .05 * e.precise_wheel_delta_y;
  Refresh();
  return true;
}
void AnalysisView::mouseDown(const visage::MouseEvent &e) {
  if (!e.isLeftButton())
    return;
  if (e.repeatClickCount() == 2) {
    ResetZoom();
    return;
  }
  previous_ = e.position;
  dragWaveform_ = e.position.y < PlotTop;
  dragReference_ = At(e.position.x, e.position.y).reference;
  dragging_ = true;
  divider_ =
      !dragWaveform_ &&
      ((Mode() == Comparison::Mirror || Mode() == Comparison::SideBySide)
           ? std::abs(e.position.x - 42 - float(split) * (width() - 54)) < 7
           : Mode() == Comparison::Stacked &&
                 std::abs(e.position.y - PlotTop -
                          float(split) * (height() - PlotTop - 29)) < 7);
}
void AnalysisView::mouseDrag(const visage::MouseEvent &e) {
  if (!dragging_)
    return;
  if (divider_) {
    split =
        Mode() == Comparison::Stacked
            ? (e.position.y - PlotTop) / std::max(1.f, height() - PlotTop - 29)
            : (e.position.x - 42) / std::max(1.f, width() - 54);
    split = std::clamp(split, .1, .9);
  } else {
    const bool horizontal =
        !dragWaveform_ &&
        (Mode() == Comparison::Mirror || Mode() == Comparison::SideBySide);
    const double pane = horizontal ? (dragReference_ ? split : 1 - split) : 1;
    const double direction =
        Mode() == Comparison::Mirror && dragReference_ ? -1 : 1;
    const double delta = (previous_.x - e.position.x) * span * direction /
                         std::max(1., (width() - 54) * pane);
    if (e.isShiftDown() && !dragWaveform_)
      (dragReference_ ? referenceOffset : modelOffset) += delta;
    else
      pan += delta;
  }
  previous_ = e.position;
  Refresh();
}
void AnalysisView::mouseUp(const visage::MouseEvent &) { dragging_ = false; }
void AnalysisView::mouseMove(const visage::MouseEvent &e) {
  pointer_ = e.position;
  hover_ =
      pointer_.x >= 42 && pointer_.y >= PlotTop && pointer_.y < height() - 29;
  redraw();
}
} // namespace drumfoundry::ui
