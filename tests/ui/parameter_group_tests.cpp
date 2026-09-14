#include "ui/parameter_panel.hpp"
#include <stdexcept>

namespace {
void Check(bool condition) {
  if (!condition)
    throw std::runtime_error("Parameter group layout regression");
}
} // namespace

void ParameterGroupTests(drumfoundry::editing::Document document) {
  using namespace drumfoundry::ui;
  const auto original = document.JsonValue();
  visage::Palette palette;
  ParameterPanel panel;
  panel.setPalette(&palette);
  for (bool right : {false, true}) {
    panel.Load(document, right);
    for (int textSize : {0, 1, 2}) {
      ConfigureTextSize(palette, textSize);
      for (int width : {240, 310, 530}) {
        panel.setBounds(0, 0, width, 400);
        panel.resized();
        const auto *content = panel.children().front();
        Check(!content->children().empty());
        float previousBottom = 0;
        unsigned groupGaps = 0;
        for (auto *row : content->children()) {
          Check(row->x() == 12 && row->right() == width - 26);
          Check(row->y() >= previousBottom && row->height() > 0);
          groupGaps += row->y() - previousBottom >= 20;
          previousBottom = row->bottom();
        }
        Check(groupGaps > 0 && panel.scrollableHeight() >= previousBottom);
        panel.setYPosition(80);
        Check(content->y() == -panel.yPosition());
        panel.setYPosition(0);
      }
    }
  }
  Check(document.JsonValue() ==
        original); // Grouping never changes the patch.
}
