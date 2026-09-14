#pragma once
#include "controls.hpp"
#include <array>

namespace drumfoundry::ui {
// Native toggle buttons keep selected backgrounds visible without hovering.
class LayoutPanel : public visage::Frame {
public:
  LayoutPanel();
  void Sync(bool spectrogram, bool modes, bool singleColumn, int textSize);
  void resized() override;
  void draw(visage::Canvas &) override;
  bool keyPress(const visage::KeyEvent &) override;
  std::function<void(int)> selected; // 0..2 toggles, 3 reset, 4..6 text size.
  std::function<void()> close;

private:
  std::array<visage::UiButton, 3> toggles_{
      {visage::UiButton("Show spectrogram"),
       visage::UiButton("Show modal editor"),
       visage::UiButton("Single-column controls")}};
  std::array<visage::UiButton, 3> sizes_{{visage::UiButton("Small"),
                                          visage::UiButton("Medium"),
                                          visage::UiButton("Large")}};
  visage::UiButton reset_{"Reset layout"}, close_{"Done"};
};
} // namespace drumfoundry::ui
