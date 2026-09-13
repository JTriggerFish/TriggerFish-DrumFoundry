#include "analysis_panel.hpp"
#include <cmath>
#include <cstdio>

namespace drumfoundry::ui {
void AnalysisPanel::RefreshTransformLabels() {
  const auto &t = request_.transform;
  fft_.setText("FFT: " + std::to_string(t.size));
  const char *window = t.window == "hann"          ? "Hann"
                       : t.window == "rectangular" ? "Rectangular"
                                                   : "Blackman–Harris";
  window_.setText(std::string("Window: ") + window);
  char label[64];
  std::snprintf(label, sizeof(label), "Overlap: %.4g%%",
                100. * (1. - double(t.hop) / t.size));
  overlap_.setText(label);
  std::snprintf(label, sizeof(label), "Render: %.3g s…", duration_.Value());
  render_.setText(label);
}
void AnalysisPanel::OverlapMenu() {
  visage::PopupMenu menu;
  for (int divisor : {2, 4, 8, 16}) {
    char label[32];
    std::snprintf(label, sizeof(label), "%.4g%%", 100. - 100. / divisor);
    menu.addOption(divisor, label)
        .select(request_.transform.hop * divisor == request_.transform.size);
  }
  menu.onSelection() = [this](int divisor) {
    request_.transform.hop = request_.transform.size / unsigned(divisor);
    Queue();
  };
  menu.show(&overlap_);
}
void AnalysisPanel::RenderMenu() {
  visage::PopupMenu menu;
  menu.addOption(0, "Match reference");
  for (int seconds : {1, 3, 6, 12, 30, 60})
    menu.addOption(seconds, std::to_string(seconds) + " s");
  menu.onSelection() = [this](int seconds) {
    if (!seconds && request_.reference.empty()) {
      if (error)
        error("Choose a reference before matching its duration");
      return;
    }
    matchLength_ = seconds == 0;
    if (seconds) {
      duration_.Set(seconds);
      view_.span = seconds;
      view_.pan = 0;
    }
    Queue();
  };
  menu.show(&render_);
}
} // namespace drumfoundry::ui
