#include "analysis_view.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
bool AnalysisView::mouseWheel(const visage::MouseEvent &e) {
  if (e.isCtrlDown() || e.isCmdDown())
    span =
        std::clamp(span * std::exp(-.12 * e.precise_wheel_delta_y), .02, 60.);
  else
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
  dragging_ = true;
  divider_ =
      (comparison == Comparison::Mirror || comparison == Comparison::SideBySide)
          ? std::abs(e.position.x - 42 - float(split) * (width() - 54)) < 7
          : comparison == Comparison::Stacked &&
                std::abs(e.position.y - 62 - float(split) * (height() - 91)) <
                    7;
}
void AnalysisView::mouseDrag(const visage::MouseEvent &e) {
  if (!dragging_)
    return;
  if (divider_) {
    split = comparison == Comparison::Stacked
                ? (e.position.y - 62) / std::max(1.f, height() - 91)
                : (e.position.x - 42) / std::max(1.f, width() - 54);
    split = std::clamp(split, .1, .9);
  } else {
    const auto a = At(previous_.x, previous_.y),
               b = At(e.position.x, e.position.y);
    if (e.isShiftDown())
      (a.reference ? referenceOffset : modelOffset) += a.time - b.time;
    else
      pan += a.time - b.time;
  }
  previous_ = e.position;
  Refresh();
}
void AnalysisView::mouseUp(const visage::MouseEvent &) { dragging_ = false; }
} // namespace drumfoundry::ui
