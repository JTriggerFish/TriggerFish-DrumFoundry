#include "workbench.hpp"
#include <algorithm>

namespace drumfoundry::ui {
float Workbench::LeftWidth() const {
  return std::clamp(float(analysis_.leftShare) * width(), 300.f,
                    std::max(300.f, width() - 420.f));
}
bool Workbench::SingleColumn() const {
  return analysis_.singleColumn ||
         LeftWidth() < 520 * TextSizeScale(analysis_.textSize);
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
  setPalette(&textPalette_);
  ApplyTheme(textPalette_, DefaultTheme());
  try {
    if (std::filesystem::exists(ThemeSettingsPath()))
      LoadTheme(ReadTheme(ThemeSettingsPath()), false);
  } catch (const std::exception &e) {
    Error(std::string("Using default colours: ") + e.what());
  }
  addChild(&layoutShade_, false);
  layoutShade_.setOnTop(true);
  layoutShade_.addChild(&layoutPanel_);
  layoutShade_.onMouseDown() = [this](const auto &) {
    layoutShade_.setVisible(false);
  };
  layoutPanel_.close = [this] { layoutShade_.setVisible(false); };
  layoutPanel_.selected = [this](int item) { ChooseLayout(item); };
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
  layoutPanel_.Sync(analysis_.showSpectrogram, analysis_.showModalEditor,
                    analysis_.singleColumn, analysis_.textSize);
  layoutShade_.setVisible(true);
  layoutPanel_.requestKeyboardFocus();
}
void Workbench::ChooseLayout(int item) {
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
    SetTextSize(0);
  } else if (item >= 4 && item <= 6)
    SetTextSize(item - 4);
  resized();
  layoutPanel_.Sync(analysis_.showSpectrogram, analysis_.showModalEditor,
                    analysis_.singleColumn, analysis_.textSize);
}
void Workbench::SetTextSize(int size) {
  analysis_.textSize = std::clamp(size, 0, 2);
  ApplyTextSize();
  resized();
}
void Workbench::ApplyTextSize() {
  ConfigureTextSize(textPalette_, analysis_.textSize);
  NativeFonts(*this);
  // Reflow even if a frame's outer bounds have not changed.
  excitation_.resized();
  resonance_.resized();
  modal_.resized();
  history_.resized();
  if (textSizeChanged)
    textSizeChanged();
}
} // namespace drumfoundry::ui
