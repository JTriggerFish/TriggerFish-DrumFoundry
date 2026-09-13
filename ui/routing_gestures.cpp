#include "routing_diagram.hpp"
#include <algorithm>
namespace drumfoundry::ui {
void RoutingDiagram::mouseDown(const visage::MouseEvent &e) {
  dragging_.clear();
  if (!e.isLeftButton() || !document_)
    return;
  if (!editable) {
    if (e.repeatClickCount() == 2 && open)
      open();
    return;
  }
  for (const auto &[id, p] : positions_.items()) {
    const auto at = Map({p.at("x"), p.at("y")});
    if (e.position.x >= at.x && e.position.x <= at.x + 124 * Scale() &&
        e.position.y >= at.y && e.position.y <= at.y + 48 * Scale()) {
      dragging_ = id;
      origin_ = e.windowPosition();
      nodeOrigin_ = {p.at("x"), p.at("y")};
      break;
    }
  }
}
void RoutingDiagram::mouseDrag(const visage::MouseEvent &e) {
  if (dragging_.empty())
    return;
  try {
    const auto delta = (e.windowPosition() - origin_) / Scale();
    const double x = std::clamp(double(nodeOrigin_.x + delta.x), 0.,
                                std::min(4096., canvasWidth_ - 124));
    const double y = std::clamp(double(nodeOrigin_.y + delta.y), 0.,
                                std::min(2048., canvasHeight_ - 48));
    document_->MoveNode(dragging_, x, y);
    positions_[dragging_] = {{"x", x}, {"y", y}};
    redraw(); // Keep the coordinate transform fixed for the whole gesture.
  } catch (const std::exception &ex) {
    if (error)
      error(ex.what());
  }
}
void RoutingDiagram::mouseUp(const visage::MouseEvent &) {
  if (dragging_.empty())
    return;
  dragging_.clear();
  Fit();
  redraw();
  if (layoutChanged)
    layoutChanged();
}
} // namespace drumfoundry::ui
