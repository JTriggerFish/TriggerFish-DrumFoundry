#include "workbench.hpp"
#include <algorithm>
#include <cstdio>

namespace drumfoundry::ui {
void Workbench::resized() {
  help_.Hide();
  const float left = LeftWidth();
  preset_.setBounds(16, 48, 150, 30);
  stop_.setBounds(174, 48, 80, 30);
  referencePlay_.setBounds(270, 48, 112, 30);
  modelPlay_.setBounds(390, 48, 100, 30);
  limiter_.setBounds(width() - 367, 48, 125, 30);
  settings_.setBounds(width() - 130, 10, 114, 28);
  layout_.setBounds(width() - 250, 10, 108, 28);
  master_.setBounds(width() - 226, 42, 210, 44);
  const float col = (left - 48) / 2;
  routingToggle_.setBounds(16, 110, left - 32, 28);
  routing_.setBounds(16, 142, left - 32, 150);
  const float controlsTop = LeftControlsTop();
  const bool kick = document_.Recipe() == "drum.kick.v1";
  const float controlsOffset = kick ? 30.f : 78.f;
  columnSplit_.setBounds(left - 4, 110, 10, height() - 221);
  right_.setBounds(left + 16, 110, width() - left - 32, height() - 221);
  LayoutRight();
  location_.setBounds(16, controlsTop + 24, col, 44);
  mute_.setBounds(32 + col, controlsTop + 24, col, 44);
  const bool single = SingleColumn();
  excitationTab_.setVisible(single);
  resonanceTab_.setVisible(single);
  excitationTab_.setBounds(16, controlsTop, col, 24);
  resonanceTab_.setBounds(32 + col, controlsTop, col, 24);
  excitationTab_.setActionButton(!resonanceSelected_);
  resonanceTab_.setActionButton(resonanceSelected_);
  excitation_.setVisible(!single || !resonanceSelected_);
  resonance_.setVisible(!single || resonanceSelected_);
  excitation_.setBounds(16, controlsTop + controlsOffset,
                        single ? left - 32 : col,
                        height() - controlsTop - controlsOffset - 111);
  resonance_.setBounds(single ? 16 : 32 + col, controlsTop + controlsOffset,
                       single ? left - 32 : col,
                       height() - controlsTop - controlsOffset - 111);
  history_.setBounds(16, height() - 100, width() - 32, 34);
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
  c.setColor(0xff10151c);
  c.fill(0, 0, width(), height());
  Label(c, "TRIGGERFISH  /  DrumFoundry", 16, 6, 520, 30, 0xffe8b755);
  if (!document_.JsonValue().is_null())
    Label(c, document_.JsonValue().value("name", "Imported patch"), 290, 6,
          std::max(0.f, width() - 560), 30);
  c.setColor(0xff293440);
  c.fill(0, 98, width(), 1);
  c.fill(left, 98, 1, height() - 140);
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
  Label(c, meter, width() - 510, 78, 495, 18,
        reduction_ > .1 ? 0xffffc65c : 0xff8799ae);
  Label(c,
        audioRunning_ ? "Audio running" : "AUDIO STOPPED — click Start audio",
        16, 79, 320, 18, audioRunning_ ? 0xff8799ae : 0xffefb178);
  Label(c, status_, 16, height() - 55, width() - 32, 24, 0xff8799ae);
  Label(c, error_, 16, height() - 29, width() - 32, 24, 0xffefb178);
}
} // namespace drumfoundry::ui
