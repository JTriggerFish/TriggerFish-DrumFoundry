#include "workbench.hpp"
#include <algorithm>

namespace drumfoundry::ui {
void Workbench::LayoutRight() {
  if (right_.width() < 160 || right_.height() < 1)
    return;
  const float w = std::max(1.f, right_.width() - 14);
  const bool narrow = w < 560;
  const float strikeHeight = narrow ? 386.f : 252.f;
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
  const float padWidth =
      narrow ? std::min(w, 360.f) : std::clamp(w * .3f, 280.f, 360.f);
  strike_.setBounds(0, top, padWidth, 180);
  const float controlsX = narrow ? 0 : padWidth + 16;
  const float controlsTop = narrow ? top + 188 : top;
  const float controlWidth = std::min(540.f, w - controlsX);
  const float buttonWidth = std::min(140.f, (controlWidth - 16) / 3);
  for (unsigned i = 0; i < implements_.size(); ++i)
    implements_[i].setBounds(controlsX + i * (buttonWidth + 8), controlsTop,
                             buttonWidth, 28);
  hardness_.setBounds(controlsX, controlsTop + 36, controlWidth, 44);
  spread_.setBounds(controlsX, controlsTop + 84, controlWidth, 44);
  const float velocityTop = narrow ? controlsTop + 132 : top + 186;
  const float velocityWidth = std::min(360.f, w - 116);
  velocity_.setBounds(0, velocityTop, velocityWidth, 44);
  fixedStrike_.setBounds(velocityWidth + 12, velocityTop + 7, 92, 30);
  const float modalTop = analysisHeight + strikeHeight;
  modal_.setBounds(0, modalTop, w,
                   std::max(minModal, contentHeight - modalTop));
  right_.setScrollableHeight(contentHeight);
}
} // namespace drumfoundry::ui
