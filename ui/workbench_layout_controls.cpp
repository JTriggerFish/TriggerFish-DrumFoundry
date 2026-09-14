#include "workbench.hpp"
#include <algorithm>

namespace drumfoundry::ui {
float Workbench::LeftWidth() const {
  return std::clamp(float(analysis_.leftShare) * width(), 300.f,
                    std::max(300.f, width() - 420.f));
}
bool Workbench::SingleColumn() const {
  return analysis_.singleColumn || LeftWidth() < 520;
}
void Workbench::SetControlWidth(float pixels) {
  if (width() <= 0)
    return;
  analysis_.leftShare = std::clamp(double(pixels / width()), .1, .85);
  resized();
}
void Workbench::SetVisualPanels(bool spectrogram, bool modes) {
  analysis_.showSpectrogram = spectrogram;
  analysis_.showModalEditor = modes;
  LayoutRight();
}
void Workbench::SetupLayout() {
  layout_.onToggle() = [this](auto *, bool) { OpenLayout(); };
  columnSplit_.vertical = true;
  columnSplit_.started = [this] { columnStart_ = LeftWidth(); };
  columnSplit_.dragged = [this](float delta) {
    SetControlWidth(columnStart_ + delta);
  };
  excitationTab_.onToggle() = [this](auto *, bool) {
    resonanceSelected_ = false;
    resized();
  };
  resonanceTab_.onToggle() = [this](auto *, bool) {
    resonanceSelected_ = true;
    resized();
  };
}
void Workbench::OpenLayout() {
  visage::PopupMenu menu;
  menu.addOption(0, "Show spectrogram").select(analysis_.showSpectrogram);
  menu.addOption(1, "Show modal editor").select(analysis_.showModalEditor);
  menu.addOption(2, "Single-column controls").select(analysis_.singleColumn);
  menu.addOption(3, "Reset layout");
  menu.onSelection() = [this](int item) {
    if (item == 0)
      analysis_.showSpectrogram = !analysis_.showSpectrogram;
    else if (item == 1)
      analysis_.showModalEditor = !analysis_.showModalEditor;
    else if (item == 2)
      analysis_.singleColumn = !analysis_.singleColumn;
    else if (item == 3) {
      analysis_.showSpectrogram = analysis_.showModalEditor = true;
      analysis_.singleColumn = false;
      analysis_.leftShare = .4;
      analysis_.analysisShare = 450. / 1100;
    }
    resized();
  };
  menu.show(&layout_);
}
} // namespace drumfoundry::ui
