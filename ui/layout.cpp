#include "workbench.hpp"
#include <algorithm>
#include <cstdio>

namespace drumfoundry::ui {
void Workbench::resized() {
  help_.Hide();
  const float left = LeftWidth();
  const float scale = TextSizeScale(analysis_.textSize);
  preset_.setBounds(16, 9, 150 * scale, 30);
  undo_.setBounds(preset_.right() + 8, 9, 52, 30);
  redo_.setBounds(undo_.right() + 4, 9, 52, 30);
  settings_.setBounds(width() - 16 - 100 * scale, 9, 100 * scale, 30);
  master_.setBounds(settings_.x() - 198, 1, 184, 44);
  limiter_.setBounds(master_.x() - 14 - 138 * scale, 9, 138 * scale, 30);
  const float col = (left - 48) / 2;
  routingToggle_.setBounds(16, 60, left - 32, 28);
  routing_.setBounds(16, 92, left - 32, 150);
  const float controlsTop = LeftControlsTop();
  const bool kick = document_.Recipe() == "drum.kick.v1";
  const bool stackedPerformance = SingleColumn() && analysis_.textSize > 0;
  const float controlsOffset =
      kick ? 30.f : (stackedPerformance ? 126.f : 78.f);
  columnSplit_.setBounds(left - 6, 60, 14, height() - 100);
  right_.setBounds(left + 16, 60, width() - left - 32, height() - 100);
  LayoutRight();
  location_.setBounds(16, controlsTop + 24,
                      stackedPerformance ? left - 32 : col, 44);
  mute_.setBounds(stackedPerformance ? 16 : 32 + col,
                  controlsTop + (stackedPerformance ? 72 : 24),
                  stackedPerformance ? left - 32 : col, 44);
  const bool single = SingleColumn();
  excitationTab_.setVisible(single);
  resonanceTab_.setVisible(single);
  excitationTab_.setText(analysis_.textSize > 0 ? "Excitation"
                                                : "Excitation / output");
  resonanceTab_.setText(analysis_.textSize > 0 ? "Resonance"
                                               : "Resonance / bloom");
  excitationTab_.setBounds(16, controlsTop, col, 24);
  resonanceTab_.setBounds(32 + col, controlsTop, col, 24);
  excitationTab_.setActionButton(!resonanceSelected_);
  resonanceTab_.setActionButton(resonanceSelected_);
  excitation_.setVisible(!single || !resonanceSelected_);
  resonance_.setVisible(!single || resonanceSelected_);
  excitation_.setBounds(16, controlsTop + controlsOffset,
                        single ? left - 32 : col,
                        height() - controlsTop - controlsOffset - 40);
  resonance_.setBounds(single ? 16 : 32 + col, controlsTop + controlsOffset,
                       single ? left - 32 : col,
                       height() - controlsTop - controlsOffset - 40);
  footer_.setBounds(16, height() - 30, width() - 32, 26);
  presetShade_.setBounds(localBounds());
  presetSave_.setBounds((width() - 550) / 2, (height() - 164) / 2, 550, 164);
  layoutShade_.setBounds(localBounds());
  layoutPanel_.setBounds(width() - 356, 60, 340, 284);
  fileShade_.setBounds(localBounds());
  files_.setBounds((width() - 640) / 2, (height() - 300) / 2, 640, 300);
  metaShade_.setBounds(localBounds());
  meta_.setBounds((width() - 700) / 2, 110, 700, 280);
  routingShade_.setBounds(localBounds());
  const float routingWidth = std::min(1120.f, width() - 40),
              routingHeight = std::min(760.f, height() - 60);
  routes_.setBounds((width() - routingWidth) / 2,
                    (height() - routingHeight) / 2, routingWidth,
                    routingHeight);
}
void Workbench::draw(visage::Canvas &c) {
  const float left = LeftWidth();
  c.setColor(colours::Background);
  c.fill(0, 0, width(), height());
  c.setColor(colours::Panel);
  c.fill(0, 0, width(), 48);
  c.fill(0, height() - 34, width(), 34);
  const float titleX = redo_.right() + 16;
  const float titleWidth = std::max(0.f, limiter_.x() - titleX - 16);
  const std::string title =
      document_.JsonValue().is_null()
          ? "DrumFoundry"
          : document_.JsonValue().value("name", "DrumFoundry");
  Label(c, ElideText(FrameFont(*this), title, titleWidth), titleX, 9,
        titleWidth, 30, colours::Heading);
  c.setColor(colours::Grid);
  c.fill(0, 48, width(), 1);
  if (!SingleColumn()) {
    Label(c, "EXCITATION & OUTPUT", 16, LeftControlsTop(), left / 2 - 20, 24);
    Label(c,
          document_.Recipe() == "drum.kick.v1" ? "THUMP & RESONANCE"
                                               : "RESONANCE & BLOOM",
          left / 2 + 8, LeftControlsTop(), left / 2 - 20, 24);
  }
  char meter[96];
  std::snprintf(meter, sizeof(meter), "Reduction %.1f dB  |  Lookahead %.2f ms",
                std::max(0., reduction_), latency_);
  footer_.Set(meter, status_, error_, reduction_ > .1);
}
} // namespace drumfoundry::ui
