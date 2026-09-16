#include "editing/curves.hpp"
#include "modal_plot.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
using namespace editing;
int ModalPlot::Hit(visage::Point p, bool handlesOnly) const {
  for (int i = int(modes_.size()) - 1; i >= 0; --i)
    if (modes_[i].level > -72 && modes_[i].frequency >= 20 &&
        std::abs(p.x - X(modes_[i].frequency)) < 8 &&
        std::abs(p.y - Y(modes_[i].level)) < 8)
      return i;
  if (handlesOnly)
    return -1;
  for (int i = int(modes_.size()) - 1; i >= 0; --i)
    if (modes_[i].level > -72 && modes_[i].frequency >= 20 &&
        std::abs(p.x - X(modes_[i].frequency)) < 8 &&
        p.y >= Y(modes_[i].level) - 8)
      return i;
  return -1;
}
void ModalPlot::Store() {
  ReplaceModes(*document_, modes_);
  edited_ = true;
  redraw();
  if (changed)
    changed();
  if (edited)
    edited();
}
void ModalPlot::mouseDown(const visage::MouseEvent &e) {
  if (!document_ || modes_.empty() || !e.isLeftButton() || e.position.x < 42 ||
      e.position.x > width() - 18 || e.position.y < 14 ||
      e.position.y > height() - 30)
    return;
  requestKeyboardFocus();
  previous_ = e.position;
  dragging_ = true;
  marquee_ = edited_ = false;
  dragModes_ = modes_; // Baseline for both moving and painting cancellation.
  try {
    const int handle = Hit(e.position, true);
    const int hit = handle >= 0                     ? handle
                    : SelectionContains(e.position) ? selected
                                                    : Hit(e.position);
    if (tool == Tool::Edit && e.repeatClickCount() == 2) {
      // Only a double-click on the circular handle deletes. Clicking the
      // empty area below a tall stem must still allow inserting a new mode.
      const int handle = Hit(e.position, true);
      if (handle >= 0) {
        SelectOnly(handle);
        Remove();
      } else {
        const int slot =
            int(InsertMode(*document_, Snap(Frequency(e.position.x)),
                           std::max(-71.9, Level(e.position.y))));
        modes_ = Modes(*document_);
        SelectOnly(slot);
        if (committed)
          committed();
      }
      dragging_ = false;
    } else if (tool != Tool::Edit) {
      Paint(e.position, e);
      return;
    } else
      BeginSelection(e, hit);
    redraw();
    if (changed)
      changed();
  } catch (const std::exception &ex) {
    dragging_ = marquee_ = false;
    redraw();
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
    if (marquee_) {
      SelectRectangle(e.position);
      return;
    }
    if (selected < 0)
      return;
    MoveSelection(e);
  } catch (const std::exception &ex) {
    if (error)
      error(ex.what());
  }
}
void ModalPlot::mouseUp(const visage::MouseEvent &e) {
  if (!e.isLeftButton())
    return;
  if (dragging_ && edited_ && committed)
    committed();
  dragging_ = marquee_ = false;
  redraw();
}
bool ModalPlot::mouseWheel(const visage::MouseEvent &e) {
  if (!document_ || selected < 0 || ModePrefix(*document_) != "resolved_")
    return false;
  const bool group = e.isCtrlDown() || e.isCmdDown();
  for (unsigned i = 0; i < modes_.size(); ++i)
    if (group ? IsSelected(i) : int(i) == selected)
      modes_[i].turbulence = std::clamp(
          modes_[i].turbulence + e.precise_wheel_delta_y * .02, 0., 2.);
  Store();
  if (committed)
    committed();
  return true;
}
bool ModalPlot::keyPress(const visage::KeyEvent &e) {
  if (e.keyCode() == visage::KeyCode::Escape) {
    const bool restore = dragging_ && edited_;
    if (restore) {
      ReplaceModes(*document_, dragModes_);
      modes_ = dragModes_;
    }
    edited_ = false;
    SelectOnly(-1);
    dragging_ = marquee_ = false;
    redraw();
    if (changed)
      changed();
    // The parent must restore the live engine and close its gesture too.
    if (restore && cancelled)
      cancelled();
    return true;
  }
  if (e.keyCode() != visage::KeyCode::Delete &&
      e.keyCode() != visage::KeyCode::Backspace)
    return false;
  Remove();
  return true;
}
void ModalPlot::Remove() {
  if (selected < 0)
    return;
  for (unsigned i = 0; i < modes_.size(); ++i)
    if (IsSelected(i))
      modes_[i].level = -72;
  SelectOnly(-1);
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
