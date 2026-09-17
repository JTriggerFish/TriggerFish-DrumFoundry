#include "workbench.hpp"
#include <algorithm>

namespace drumfoundry::ui {
// Two performance columns beside the pad on wide layouts, below it on smaller
// ones. Return the occupied height so analysis/modal resizing stays independent.
float Workbench::LayoutStrike(float top, float w) {
  const bool beside = w >= 1000, columns = w >= 560;
  const float padWidth = beside ? std::clamp(w * .26f, 280.f, 360.f)
                                : std::min(w, 360.f);
  strike_.setBounds(0, top, padWidth, 180);
  freezeStrike_.setBounds(0, top + 188, padWidth, 30);

  const float controlsX = beside ? padWidth + 16 : 0;
  const float controlsTop = beside ? top : top + 234;
  const float controlWidth = columns ? (w - controlsX - 16) / 2 : w;
  const float buttonWidth = (w - controlsX - 16) / 3;
  for (unsigned i = 0; i < implements_.size(); ++i)
    implements_[i].setBounds(controlsX + i * (buttonWidth + 8), controlsTop,
                             buttonWidth, 28);
  const float row = std::max(44.f, hardness_.PreferredHeight(controlWidth));
  hardness_.setBounds(controlsX, controlsTop + 36, controlWidth, row);
  spread_.setBounds(controlsX, hardness_.bottom() + 4, controlWidth, row);
  const float playingX = columns ? controlsX + controlWidth + 16 : 0;
  const float playingTop = columns ? controlsTop + 36 : spread_.bottom() + 12;
  playing_.setBounds(playingX, playingTop, controlWidth, 1);
  playing_.resized(); // Measure inline controls at the current width/text size.
  const float playingHeight = playing_.scrollableHeight();
  playing_.setVisible(playingHeight > 0);
  playing_.setBounds(playingX, playingTop, controlWidth, playingHeight);
  playing_.setYPosition(0); // The outer analysis column owns scrolling here.
  const float bottom = std::max({freezeStrike_.bottom(), spread_.bottom(),
                                 playingHeight > 0 ? playing_.bottom() : 0.f});
  return bottom - top + 20;
}
void Workbench::LayoutRight() {
  if (right_.width() < 160 || right_.height() < 1)
    return;
  const float w = std::max(1.f, right_.width() - 14);
  const float strikeHeight = 20 + LayoutStrike(0, w);
  const bool modes = analysis_.showModalEditor && modal_.Available();
  modal_.setVisible(modes);
  modal_.setBounds(0, modal_.y(), w, modal_.height());
  analysis_.setBounds(0, 0, w, analysis_.height());
  const float minAnalysis = analysis_.MinimumHeight();
  const float minModal = modes ? modal_.MinimumHeight() : 0;
  // The divider may grow scrollable content; minimum editor heights must not
  // pin it in place on a small screen.
  flexibleHeight_ = std::max(std::max(1100.f, right_.height()) - strikeHeight,
                             (minAnalysis + 500) / .9f);
  const float analysisHeight =
      !analysis_.showSpectrogram
          ? minAnalysis
          : std::max(minAnalysis,
                     float(analysis_.analysisShare) * flexibleHeight_);
  const float contentHeight =
      std::max(right_.height(), analysisHeight + strikeHeight + minModal);
  analysis_.setBounds(0, 0, w, analysisHeight);
  analysisSplit_.setVisible(analysis_.showSpectrogram);
  analysisSplit_.setBounds(0, analysisHeight, w, 14);
  const float top = analysisHeight + 20;
  LayoutStrike(top, w);
  const float modalTop = analysisHeight + strikeHeight;
  modal_.setBounds(0, modalTop, w,
                   std::max(minModal, contentHeight - modalTop));
  right_.setScrollableHeight(contentHeight);
}
} // namespace drumfoundry::ui
