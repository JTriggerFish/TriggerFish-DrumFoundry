#pragma once
#include "controls.hpp"
namespace drumfoundry::ui {
// Native button interaction with a font-independent play triangle.
class PlayButton : public HelpButton {
public:
  PlayButton() : HelpButton("") {
    help = "Play the selected reference sample.";
  }
  void draw(visage::Canvas &c, float hover) override {
    drawBackground(c, hover);
    c.setColor(colours::Accent);
    const float x = width() * .5f, y = height() * .5f;
    c.triangle(x - 4, y - 6, x + 6, y, x - 4, y + 6);
  }
};
} // namespace drumfoundry::ui
