#include "decay_editor.hpp"
#include <cmath>
namespace drumfoundry::ui {
using namespace editing;
int DecayEditor::Hit(visage::Point position) const {
  for (auto p : DecayKnots(document_))
    if (std::hypot(position.x - X(p.frequency), position.y - Y(p.seconds)) < 11)
      return p.slot;
  const float mx = (36 + width() - 14) / 2;
  if (std::hypot(position.x - mx,
                 position.y - Y(DecayAt(document_, Frequency(mx)))) < 12)
    return 8;
  return -1;
}
void DecayEditor::mouseDown(const visage::MouseEvent &e) {
  if (!e.isLeftButton() || e.position.y > height() - 164)
    return;
  try {
    drag_ = Hit(e.position);
    previous_ = e.position;
    if (drag_ >= 0 && drag_ < 8)
      selected_ = drag_;
    if (e.repeatClickCount() == 2) {
      if (drag_ == -1)
        selected_ = InsertDecay(document_, Frequency(e.position.x),
                                Seconds(e.position.y));
      else if (drag_ > 0 && drag_ < 7) {
        DeleteDecay(document_, drag_);
        selected_ = 0;
      } else if (drag_ == 0 || drag_ == 7) {
        const auto key = "body_decay_seconds_" + std::to_string(drag_);
        document_.Set(key, document_.Description(key).initial);
      }
      drag_ = -1;
      if (committed)
        committed();
    }
    Sync();
  } catch (const std::exception &ex) {
    if (error)
      error(ex.what());
  }
}
void DecayEditor::mouseDrag(const visage::MouseEvent &e) {
  if (drag_ < 0)
    return;
  const float fine = e.isShiftDown() ? .1f : 1.f;
  try {
    if (drag_ == 8) {
      const double middle =
          DecayAt(document_, Frequency((36 + width() - 14) / 2));
      const double next =
          Seconds(Y(middle) + (e.position.y - previous_.y) * fine);
      ShiftDecay(document_, std::log2(next / middle));
    } else {
      for (auto p : DecayKnots(document_))
        if (p.slot == drag_) {
          SetDecay(
              document_, drag_,
              Frequency(X(p.frequency) + (e.position.x - previous_.x) * fine),
              Seconds(Y(p.seconds) + (e.position.y - previous_.y) * fine));
        }
    }
    previous_ = e.position;
    Sync();
    if (changed)
      changed();
  } catch (const std::exception &ex) {
    if (error)
      error(ex.what());
  }
}
void DecayEditor::mouseUp(const visage::MouseEvent &e) {
  if (e.isLeftButton() && drag_ >= 0 && committed)
    committed();
  drag_ = -1;
}
} // namespace drumfoundry::ui
