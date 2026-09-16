#include "editing/curves.hpp"
#include "modal_plot.hpp"
#include <algorithm>
#include <cmath>

namespace drumfoundry::ui {
bool ModalPlot::IsSelected(unsigned slot) const {
  return slot < selection_.size() && selection_[slot];
}
unsigned ModalPlot::SelectionCount() const {
  return unsigned(std::count(selection_.begin(), selection_.end(), true));
}
void ModalPlot::SelectOnly(int slot) {
  selection_.assign(modes_.size(), false);
  selected = slot;
  if (slot >= 0)
    selection_[slot] = true;
}
void ModalPlot::BeginSelection(const visage::MouseEvent &e, int hit) {
  origin_ = corner_ = e.position;
  movement_ = {};
  widthMovement_ = 0;
  marquee_ = hit < 0;
  if (marquee_) {
    if (!e.isShiftDown())
      SelectOnly(-1);
    selectionBefore_ = selection_;
  } else {
    if (!IsSelected(unsigned(hit))) {
      if (!e.isShiftDown())
        SelectOnly(hit);
      else
        selection_[hit] = true;
    }
    selected = hit;
    dragModes_ = modes_;
  }
}
void ModalPlot::SelectRectangle(visage::Point p) {
  corner_ = {std::clamp(p.x, 42.f, std::max(42.f, width() - 18)),
             std::clamp(p.y, 14.f, std::max(14.f, height() - 30))};
  const float left = std::min(origin_.x, corner_.x);
  const float right = std::max(origin_.x, corner_.x);
  const float top = std::min(origin_.y, corner_.y);
  const float bottom = std::max(origin_.y, corner_.y);
  selection_ = selectionBefore_;
  selected = -1;
  for (unsigned i = 0; i < modes_.size(); ++i) {
    const auto &m = modes_[i];
    // The handle (not its full-height stem) must lie inside the rectangle.
    if (m.level > -72 && m.frequency >= 20 && X(m.frequency) >= left &&
        X(m.frequency) <= right && Y(m.level) >= top && Y(m.level) <= bottom)
      selection_[i] = true;
    if (selection_[i])
      selected = int(i);
  }
  redraw();
  if (changed)
    changed();
}
void ModalPlot::MoveSelection(const visage::MouseEvent &e) {
  const float fine = e.isShiftDown() ? .1f : 1.f;
  if ((e.isCtrlDown() || e.isCmdDown()) &&
      editing::ModePrefix(*document_) == "resolved_") {
    widthMovement_ += (e.position.x - previous_.x) * fine;
    for (unsigned i = 0; i < modes_.size(); ++i)
      if (IsSelected(i))
        modes_[i].turbulence =
            std::clamp(dragModes_[i].turbulence + widthMovement_ / 100, 0., 2.);
  } else {
    movement_.x += (e.position.x - previous_.x) * fine;
    movement_.y += (e.position.y - previous_.y) * fine;
    double shift = movement_.x / std::max(1.f, width() - 60) * std::log(750.);
    double gain = -78. * movement_.y / std::max(1.f, height() - 44);
    // Snap only the grabbed handle; preserve the group's frequency ratios.
    if (guide && snap && movement_.x != 0) {
      const double f = dragModes_[selected].frequency;
      shift = std::log(Snap(f * std::exp(shift)) / f);
    }
    double low = -1e9, high = 1e9, quiet = -1e9, loud = 1e9;
    for (unsigned i = 0; i < dragModes_.size(); ++i) {
      if (!IsSelected(i))
        continue;
      const auto &m = dragModes_[i];
      low = std::max(low, std::log(20 / m.frequency));
      high = std::min(high, std::log(15000 / m.frequency));
      quiet = std::max(quiet, -71.9 - m.level);
      loud = std::min(loud, 6 - m.level);
    }
    shift = std::clamp(shift, low, high);
    gain = std::clamp(gain, quiet, loud);
    for (unsigned i = 0; i < modes_.size(); ++i) {
      if (!IsSelected(i))
        continue;
      modes_[i].frequency =
          std::clamp(dragModes_[i].frequency * std::exp(shift), 20., 15000.);
      modes_[i].level = std::clamp(dragModes_[i].level + gain, -71.9, 6.);
    }
  }
  previous_ = e.position;
  Store();
}
void ModalPlot::DrawSelection(visage::Canvas &c) {
  if (!marquee_ && SelectionCount() < 2)
    return;
  const auto bounds = SelectionBounds();
  const float x = marquee_ ? std::min(origin_.x, corner_.x) : bounds.x();
  const float y = marquee_ ? std::min(origin_.y, corner_.y) : bounds.y();
  const float w = marquee_ ? std::abs(origin_.x - corner_.x) : bounds.width();
  const float h = marquee_ ? std::abs(origin_.y - corner_.y) : bounds.height();
  c.setColor(
      c.color(colours::Secondary).withMultipliedAlpha(marquee_ ? .12f : .06f));
  c.fill(x, y, w, h);
  c.setColor(colours::Secondary);
  c.fill(x, y, w, 1);
  c.fill(x, y + h, w, 1);
  c.fill(x, y, 1, h);
  c.fill(x + w, y, 1, h);
}
visage::Bounds ModalPlot::SelectionBounds() const {
  float left = width(), right = 0, top = height();
  for (unsigned i = 0; i < modes_.size(); ++i) {
    if (!IsSelected(i))
      continue;
    left = std::min(left, X(modes_[i].frequency) - 8);
    right = std::max(right, X(modes_[i].frequency) + 8);
    top = std::min(top, Y(modes_[i].level) - 8);
  }
  left = std::max(42.f, left);
  right = std::min(width() - 18, right);
  top = std::max(14.f, top);
  return {left, top, std::max(0.f, right - left), std::max(0.f, Y(-72) - top)};
}
bool ModalPlot::SelectionContains(visage::Point p) const {
  if (SelectionCount() > 1 && SelectionBounds().contains(p))
    return true;
  // Use the same Gaussian as drawing, including the linear-Hz cloud variant.
  const double frequency = Frequency(p.x);
  for (unsigned i = 0; i < modes_.size(); ++i) {
    if (!IsSelected(i))
      continue;
    const auto &m = modes_[i];
    const double spread = Spread(m);
    if (spread <= 0)
      continue;
    const double distance =
        document_->Value("field_distribution") == 4
            ? (frequency - m.frequency) / (24.7 * (1 + .00437 * m.frequency))
            : editing::Erb(frequency) - editing::Erb(m.frequency);
    const double offset = distance / spread;
    if (std::abs(offset) <= 3 &&
        p.y >= Y(-72 + (m.level + 72) * std::exp(-.5 * offset * offset)))
      return true;
  }
  return false;
}
} // namespace drumfoundry::ui
