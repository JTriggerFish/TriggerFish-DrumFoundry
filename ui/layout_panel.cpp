#include "layout_panel.hpp"

namespace drumfoundry::ui {
LayoutPanel::LayoutPanel() {
  for (unsigned i = 0; i < 3; ++i) {
    addChild(&toggles_[i]);
    addChild(&sizes_[i]);
    toggles_[i].onToggle() = [this, i](auto *, bool) {
      if (selected)
        selected(int(i));
    };
    sizes_[i].onToggle() = [this, i](auto *, bool) {
      if (selected)
        selected(int(i) + 4);
    };
  }
  addChild(&reset_);
  addChild(&close_);
  reset_.onToggle() = [this](auto *, bool) {
    if (selected)
      selected(3);
  };
  close_.onToggle() = [this](auto *, bool) {
    if (close)
      close();
  };
}
void LayoutPanel::Sync(bool spectrogram, bool modes, bool single, int size) {
  const bool enabled[]{spectrogram, modes, single};
  for (unsigned i = 0; i < 3; ++i) {
    toggles_[i].setActionButton(enabled[i]);
    sizes_[i].setActionButton(int(i) == size);
  }
}
void LayoutPanel::resized() {
  for (unsigned i = 0; i < 3; ++i) {
    toggles_[i].setBounds(16, 42 + 38 * i, width() - 32, 30);
    const float w = (width() - 48) / 3;
    sizes_[i].setBounds(16 + (w + 8) * i, 190, w, 30);
  }
  reset_.setBounds(16, 238, 146, 30);
  close_.setBounds(width() - 108, 238, 92, 30);
}
void LayoutPanel::draw(visage::Canvas &c) {
  c.setColor(colours::Grid);
  c.roundedRectangle(0, 0, width(), height(), 8);
  c.setColor(colours::Panel);
  c.roundedRectangle(1, 1, width() - 2, height() - 2, 8);
  Label(c, "LAYOUT", 16, 8, width() - 32, 26, colours::Heading);
  Label(c, "Text size", 16, 159, width() - 32, 26);
}
bool LayoutPanel::keyPress(const visage::KeyEvent &event) {
  if (event.keyCode() != visage::KeyCode::Escape)
    return false;
  if (close)
    close();
  return true;
}
} // namespace drumfoundry::ui
