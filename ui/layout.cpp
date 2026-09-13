#include "workbench.hpp"
#include <algorithm>
#include <cstdio>

namespace drumfoundry::ui {
void Workbench::resized() {
  help_.Hide();
  const float left = std::min(640.f, width() * .46f);
  preset_.setBounds(16, 48, 150, 30);
  stop_.setBounds(174, 48, 80, 30);
  referencePlay_.setBounds(270, 48, 112, 30);
  modelPlay_.setBounds(390, 48, 100, 30);
  limiter_.setBounds(width() - 270, 48, 125, 30);
  settings_.setBounds(width() - 130, 10, 114, 28);
  master_.setBounds(width() - 490, 42, 200, 44);
  const float col = (left - 48) / 2;
  routingToggle_.setBounds(16, 110, left - 32, 28);
  routing_.setBounds(16, 142, left - 32, 150);
  const float controlsTop = LeftControlsTop();
  right_.setBounds(left + 16, 110, width() - left - 32, height() - 221);
  LayoutRight();
  location_.setBounds(16, controlsTop + 24, col, 44);
  mute_.setBounds(32 + col, controlsTop + 24, col, 44);
  excitation_.setBounds(16, controlsTop + 78, col,
                        height() - controlsTop - 189);
  resonance_.setBounds(32 + col, controlsTop + 78, col,
                       height() - controlsTop - 189);
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
void Workbench::LayoutRight() {
  if (right_.width() < 160 || right_.height() < 1)
    return;
  const float contentWidth = std::max(1.f, right_.width() - 14);
  const float contentHeight = std::max(1350.f, right_.height());
  const float flexible = contentHeight - 210;
  const float analysisHeight = std::clamp(
      float(analysis_.analysisShare) * flexible, 490.f, flexible - 520);
  analysis_.setBounds(0, 0, contentWidth, analysisHeight);
  analysisSplit_.setBounds(0, analysisHeight, contentWidth, 10);
  const float padWidth = std::clamp(contentWidth * .3f, 160.f, 360.f);
  const float controlsX = padWidth + 16;
  const float controlsWidth = contentWidth - controlsX;
  const float top = analysisHeight + 14;
  strike_.setBounds(0, top, padWidth, 138);
  const float buttonWidth = std::min(140.f, (controlsWidth - 16) / 3);
  for (unsigned i = 0; i < implements_.size(); ++i)
    implements_[i].setBounds(controlsX + i * (buttonWidth + 8), top,
                             buttonWidth, 28);
  const float controlWidth = std::min(540.f, controlsWidth);
  hardness_.setBounds(controlsX, top + 36, controlWidth, 44);
  spread_.setBounds(controlsX, top + 84, controlWidth, 44);
  velocity_.setBounds(0, top + 144, std::min(360.f, contentWidth - 116), 44);
  fixedStrike_.setBounds(std::min(360.f, contentWidth - 116) + 12, top + 151,
                         92, 30);
  const float modalTop = analysisHeight + 210;
  modal_.setBounds(0, modalTop, contentWidth, contentHeight - modalTop);
  right_.setScrollableHeight(contentHeight);
}
void Workbench::draw(visage::Canvas &c) {
  const float left = std::min(640.f, width() * .46f);
  c.setColor(0xff10151c);
  c.fill(0, 0, width(), height());
  Label(c, "TRIGGERFISH  /  DrumFoundry", 16, 6, 520, 30, 0xffe8b755);
  if (!document_.JsonValue().is_null())
    Label(c, document_.JsonValue().value("name", "Imported patch"), 290, 6,
          width() - 450, 30);
  c.setColor(0xff293440);
  c.fill(0, 98, width(), 1);
  c.fill(left, 98, 1, height() - 140);
  Label(c, "EXCITATION & OUTPUT", 16, LeftControlsTop(), left / 2 - 20, 24);
  Label(c, "RESONANCE & BLOOM", left / 2 + 8, LeftControlsTop(), left / 2 - 20,
        24);
  char meter[96];
  std::snprintf(meter, sizeof(meter), "Reduction %.1f dB  |  Lookahead %.2f ms",
                std::max(0., reduction_), latency_);
  Label(c, meter, width() - 510, 78, 495, 18,
        reduction_ > .1 ? 0xffffc65c : 0xff8799ae);
  Label(c,
        audioRunning_ ? "Audio running" : "AUDIO STOPPED — click Start audio",
        16, 79, 450, 18, audioRunning_ ? 0xff8799ae : 0xffefb178);
  Label(c, status_, 16, height() - 55, width() - 32, 24, 0xff8799ae);
  Label(c, error_, 16, height() - 29, width() - 32, 24, 0xffefb178);
}
} // namespace drumfoundry::ui
