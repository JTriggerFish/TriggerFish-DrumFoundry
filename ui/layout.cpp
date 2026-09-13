#include "workbench.hpp"
#include <algorithm>
#include <cstdio>

namespace drumfoundry::ui {
void Workbench::resized() {
  const float left = std::min(640.f, width() * .46f);
  preset_.setBounds(16, 48, 150, 30);
  stop_.setBounds(174, 48, 80, 30);
  referencePlay_.setBounds(270, 48, 112, 30);
  modelPlay_.setBounds(390, 48, 100, 30);
  limiter_.setBounds(width() - 270, 48, 125, 30);
  settings_.setBounds(width() - 130, 10, 114, 28);
  master_.setBounds(width() - 510, 42, 220, 44);
  const float col = (left - 48) / 2;
  right_.setBounds(left + 16, 110, width() - left - 32, height() - 221);
  LayoutRight();
  location_.setBounds(16, 134, col, 44);
  mute_.setBounds(32 + col, 134, col, 44);
  excitation_.setBounds(16, 188, col, height() - 299);
  resonance_.setBounds(32 + col, 188, col, height() - 299);
  history_.setBounds(16, height() - 100, width() - 32, 34);
  fileShade_.setBounds(localBounds());
  files_.setBounds((width() - 640) / 2, (height() - 300) / 2, 640, 300);
  metaShade_.setBounds(localBounds());
  meta_.setBounds((width() - 700) / 2, 110, 700, 280);
}
void Workbench::LayoutRight() {
  if (right_.width() < 160 || right_.height() < 1)
    return;
  const float contentWidth = std::max(1.f, right_.width() - 14);
  const float contentHeight = std::max(1350.f, right_.height());
  const float flexible = contentHeight - 250;
  const float analysisHeight = std::clamp(
      float(analysis_.analysisShare) * flexible, 340.f, flexible - 520);
  analysis_.setBounds(0, 0, contentWidth, analysisHeight);
  analysisSplit_.setBounds(0, analysisHeight, contentWidth, 10);
  strike_.setBounds(0, analysisHeight + 10, contentWidth, 90);
  const float buttonWidth = (contentWidth - 16) / 3;
  for (unsigned i = 0; i < implements_.size(); ++i)
    implements_[i].setBounds(i * (buttonWidth + 8), analysisHeight + 108,
                             buttonWidth, 28);
  hardness_.setBounds(0, analysisHeight + 144, contentWidth, 44);
  const float performanceWidth = (contentWidth - 116) / 2;
  velocity_.setBounds(0, analysisHeight + 192, performanceWidth, 44);
  spread_.setBounds(performanceWidth + 12, analysisHeight + 192,
                    performanceWidth, 44);
  fixedStrike_.setBounds(contentWidth - 92, analysisHeight + 202, 92, 30);
  const float modalTop = analysisHeight + 250;
  modal_.setBounds(0, modalTop, contentWidth, contentHeight - modalTop);
  right_.setScrollableHeight(contentHeight);
}
void Workbench::draw(visage::Canvas &c) {
  const float left = std::min(640.f, width() * .46f);
  c.setColor(0xff10151c);
  c.fill(0, 0, width(), height());
  Label(c, "TRIGGERFISH  /  DrumFoundry", 16, 6, 520, 30, 0xffe8b755);
  c.setColor(0xff293440);
  c.fill(0, 98, width(), 1);
  c.fill(left, 98, 1, height() - 140);
  Label(c, "EXCITATION & OUTPUT", 16, 110, left / 2 - 20, 24);
  Label(c, "RESONANCE & BLOOM", left / 2 + 8, 110, left / 2 - 20, 24);
  char meter[96];
  std::snprintf(meter, sizeof(meter), "Reduction %.1f dB  |  Lookahead %.2f ms",
                reduction_, latency_);
  Label(c, meter, width() - 510, 78, 495, 18,
        reduction_ > .1 ? 0xffffc65c : 0xff8799ae);
  Label(c, status_, 16, height() - 55, width() - 32, 24, 0xff8799ae);
  Label(c, error_, 16, height() - 29, width() - 32, 24, 0xffefb178);
}
} // namespace drumfoundry::ui
