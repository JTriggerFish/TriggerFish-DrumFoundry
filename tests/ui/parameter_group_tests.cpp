#include "ui/parameter_panel.hpp"
#include "ui/decay_hold_panel.hpp"
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
  Slider location("Strike location", 0, 1, .5), mute("Hand mute", 0, 1, 0);
  DecayHoldPanel hold;
  ParameterPanel panel;
  panel.strikeLocation = &location;
  panel.handMute = &mute;
  panel.holdDecay = &hold;
  panel.meta = [](bool) {};
  panel.setPalette(&palette);
  for (int column : {0, 1, 2}) {
    const bool right = column == 1;
    panel.playingOnly = column == 2;
    panel.Load(document, right);
    const bool playing = panel.playingOnly && document.Recipe() != "drum.kick.v1";
    for (const auto &p : document.Parameters())
      if (p.key == "hat_openness")
        Check(drumfoundry::editing::Section(p) == "Playing" &&
              !drumfoundry::editing::RightColumn(p));
    Check(bool(location.parent()) == playing);
    Check(bool(mute.parent()) == (playing && document.Recipe() == "metal.cymbal.v1"));
    for (int textSize : {0, 1, 2}) {
      ConfigureTextSize(palette, textSize);
      for (int width : {240, 310, 530}) {
        panel.setBounds(0, 0, width, 400);
        panel.resized();
        const auto *content = panel.children().front();
        Check(!content->children().empty());
        float previousBottom = 0;
        unsigned groupGaps = 0;
        bool afterBloom = false;
        for (auto *row : content->children()) {
          Check(row->x() == (panel.playingOnly ? 0 : 12) &&
                row->right() == (panel.playingOnly ? width : width - 26));
          Check(row->y() >= previousBottom && row->height() > 0);
          if (afterBloom) Check(row->y() - previousBottom == 4);
          afterBloom = row == &hold;
          if (afterBloom) {
            Check(row->height() == 34 && row->children().size() == 2);
            const auto *a = row->children()[0], *b = row->children()[1];
            Check(a->y() == b->y() && a->width() == b->width());
            Check(b->x() == a->right() + 8 && b->right() == row->width());
          }
          groupGaps += row->y() - previousBottom >= 20;
          previousBottom = row->bottom();
        }
        Check((panel.playingOnly || groupGaps > 0) &&
              panel.scrollableHeight() >= previousBottom);
        panel.setYPosition(80);
        Check(content->y() == -panel.yPosition());
        panel.setYPosition(0);
      }
    }
  }
  Check(document.JsonValue() ==
        original); // Grouping never changes the patch.
}
