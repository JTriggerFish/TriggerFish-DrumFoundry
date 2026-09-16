#include "modal_panel.hpp"
#include "toolbar_layout.hpp"
namespace drumfoundry::ui {
float ModalPanel::LayoutTools() {
  ToolbarLayout tools(width(), 26, 46);
  tools.Place(tool_, 150);
  tools.Place(clear_, 64);
  tools.Place(generate_, 128);
  tools.Place(brush_, 190, 44);
  ToolbarLayout guide(width(), tools.Bottom(), 46);
  guide.Place(guide_, 170);
  guide.Place(snap_, 92);
  guide.Place(guidePitch_, 200, 44);
  series_.setVisible(showSeries_ && available_);
  const float seriesHeight = width() < 560 ? 264.f : 210.f;
  series_.setBounds(0, guide.Bottom() + 8, width(), seriesHeight);
  return guide.Bottom() + 8 + (showSeries_ ? seriesHeight + 10 : 0);
}
float ModalPanel::MinimumHeight() { return LayoutTools() + 350; }
void ModalPanel::resized() {
  const float top = LayoutTools();
  plot_.setBounds(0, top, width(), std::max(1.f, height() - top - 150));
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
  const auto count = plot_.SelectionCount();
  Label(c,
        count > 1
            ? "MODAL PACKET DESIGN / " + std::to_string(count) + " selected"
            : "MODAL PACKET DESIGN",
        0, 0, width(), 22, colours::Heading);
  if (!available_) {
    Label(c,
          "This recipe uses its membrane body controls instead of painted "
          "modes.",
          0, 34, width(), 24);
    return;
  }
  const auto hint = plot_.tool == ModalPlot::Tool::Edit
                        ? "Drag: select/move · Ctrl-scroll: width · Del: delete"
                    : plot_.tool == ModalPlot::Tool::Paint
                        ? "Drag to paint new modes; select & move to reposition"
                        : "Drag to reshape levels; select & move to add/delete";
  Label(c, hint, 0, height() - 30, width() - 130, 24, colours::Muted);
}
} // namespace drumfoundry::ui
