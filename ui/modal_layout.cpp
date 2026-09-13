#include "modal_panel.hpp"
namespace drumfoundry::ui {
void ModalPanel::resized() {
  tool_.setBounds(0, 26, 150, 28);
  clear_.setBounds(158, 26, 64, 28);
  generate_.setBounds(230, 26, 128, 28);
  brush_.setBounds(374, 18, width() - 374, 44);
  guide_.setBounds(0, 64, 170, 28);
  snap_.setBounds(178, 64, 92, 28);
  guidePitch_.setBounds(290, 58, width() - 290, 44);
  series_.setVisible(showSeries_ && available_);
  series_.setBounds(0, 108, width(), 210);
  const float top = showSeries_ ? 328 : 110;
  plot_.setBounds(0, top, width(), height() - top - 150);
  const float col = (width() - 12) / 2;
  frequency_.setBounds(0, height() - 130, col, 44);
  level_.setBounds(col + 12, height() - 130, col, 44);
  noisiness_.setBounds(0, height() - 82, col, 44);
  allocation_.setBounds(col + 12, height() - 82, col, 44);
  remove_.setBounds(width() - 120, height() - 30, 120, 28);
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &plot_, &tool_, &clear_, &remove_, &generate_, &guide_, &snap_,
           &brush_, &guidePitch_, &frequency_, &level_, &noisiness_,
           &allocation_})
    frame->setVisible(available_);
}
void ModalPanel::draw(visage::Canvas &c) {
  Label(c, "MODAL PACKET DESIGN", 0, 0, width(), 22, 0xffe8b755);
  if (!available_) {
    Label(
        c,
        "This recipe uses its membrane body controls instead of painted modes.",
        0, 34, width(), 24);
    return;
  }
  Label(c, "Double-click: add/delete · Ctrl-drag: width · Shift: fine", 0,
        height() - 30, width() - 130, 24, 0xff8799ae);
}
} // namespace drumfoundry::ui
