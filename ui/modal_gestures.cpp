#include "editing/curves.hpp"
#include "modal_plot.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
using namespace editing;
int ModalPlot::Hit(visage::Point p) const {
  for (int i = int(modes_.size()) - 1; i >= 0; --i)
    if (modes_[i].level > -72 && std::abs(p.x - X(modes_[i].frequency)) < 8 &&
        p.y >= Y(modes_[i].level) - 8)
      return i;
  return -1;
}
void ModalPlot::Store() {
  ReplaceModes(*document_, modes_);
  redraw();
  if (changed)
    changed();
}
void ModalPlot::mouseDown(const visage::MouseEvent &e) {
  if (!document_ || modes_.empty() || !e.isLeftButton() || e.position.x < 42 ||
      e.position.y > height() - 30)
    return;
  requestKeyboardFocus();
  previous_ = e.position;
  dragging_ = true;
  try {
    if (tool != Tool::Edit) {
      Paint(e.position, e);
      return;
    }
    selected = Hit(e.position);
    if (e.repeatClickCount() == 2) {
      if (selected >= 0)
        Remove();
      else {
        selected = int(InsertMode(*document_, Snap(Frequency(e.position.x)),
                                  std::max(-71.9, Level(e.position.y))));
        modes_ = Modes(*document_);
        if (committed)
          committed();
      }
      dragging_ = false;
    }
    redraw();
    if (changed)
      changed();
  } catch (const std::exception &ex) {
    dragging_ = false;
    if (error)
      error(ex.what());
  }
}
void ModalPlot::mouseDrag(const visage::MouseEvent &e) {
  if (!dragging_)
    return;
  try {
    if (tool != Tool::Edit) {
      Paint(e.position, e);
      return;
    }
    if (selected < 0)
      return;
    auto &m = modes_[selected];
    if ((e.isCtrlDown() || e.isCmdDown()) &&
        ModePrefix(*document_) == "resolved_")
      m.turbulence =
          std::clamp(m.turbulence + (e.position.x - previous_.x) / 100, 0., 2.);
    else {
      const float fine = e.isShiftDown() ? .1f : 1.f;
      m.frequency =
          Snap(Frequency(X(m.frequency) + (e.position.x - previous_.x) * fine));
      m.level = Level(Y(m.level) + (e.position.y - previous_.y) * fine);
    }
    previous_ = e.position;
    Store();
  } catch (const std::exception &ex) {
    if (error)
      error(ex.what());
  }
}
void ModalPlot::mouseUp(const visage::MouseEvent &e) {
  if (e.isLeftButton() && dragging_ && committed)
    committed();
  dragging_ = false;
}
bool ModalPlot::mouseWheel(const visage::MouseEvent &e) {
  if (!document_ || selected < 0 || ModePrefix(*document_) != "resolved_")
    return false;
  modes_[selected].turbulence = std::clamp(
      modes_[selected].turbulence + e.precise_wheel_delta_y * .02, 0., 2.);
  Store();
  if (committed)
    committed();
  return true;
}
bool ModalPlot::keyPress(const visage::KeyEvent &e) {
  if (e.keyCode() != visage::KeyCode::Delete &&
      e.keyCode() != visage::KeyCode::Backspace)
    return false;
  Remove();
  return true;
}
void ModalPlot::Remove() {
  if (selected < 0)
    return;
  modes_[selected].level = -72;
  selected = -1;
  Store();
  if (committed)
    committed();
}
void ModalPlot::SnapAll() {
  if (!document_)
    return;
  for (auto &m : modes_)
    if (m.level > -72)
      m.frequency = Snap(m.frequency);
  Store();
  if (committed)
    committed();
}
} // namespace drumfoundry::ui
