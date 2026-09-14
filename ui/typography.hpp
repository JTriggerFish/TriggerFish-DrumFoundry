#pragma once
#include <visage/ui.h>

namespace drumfoundry::ui {
// Per-editor palette value, independent of OS DPI and other plugin instances.
extern const visage::theme::ValueId TextScale;
float TextSizeScale(int size);
visage::Font FrameFont(const visage::Frame &, float size = 13);
void ConfigureTextSize(visage::Palette &, int size);
// Keep values readable when a narrow slider cannot fit its full caption.
std::string ElideText(const visage::Font &, const std::string &, float width);
} // namespace drumfoundry::ui
