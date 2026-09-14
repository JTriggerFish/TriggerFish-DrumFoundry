#include "typography.hpp"
#include "controls.hpp"
#include <algorithm>

namespace drumfoundry::ui {
VISAGE_THEME_VALUE(TextScale, 1.f);
float TextSizeScale(int size) {
  return (13.f + 2.f * std::clamp(size, 0, 2)) / 13.f;
}
visage::Font FrameFont(const visage::Frame &frame, float size) {
  return Font(size * frame.paletteValue(TextScale));
}
std::string ElideText(const visage::Font &font, const std::string &text,
                      float width) {
  auto chars = visage::String(text).toUtf32();
  if (font.stringWidth(chars) <= width)
    return text;
  const float available = width - font.stringWidth(U"…", 1);
  if (available < 0)
    return {};
  std::size_t low = 0, high = chars.size();
  while (low < high) {
    const auto middle = (low + high + 1) / 2;
    if (font.stringWidth(chars.substr(0, middle)) <= available)
      low = middle;
    else
      high = middle - 1;
  }
  return visage::String(chars.substr(0, low) + U"…").toUtf8();
}
void ConfigureTextSize(visage::Palette &palette, int size) {
  const float scale = TextSizeScale(size);
  palette.setValue(TextScale, scale);
  // Visage exposes these built-in theme values through its named registry.
  const auto values = visage::theme::ValueId::nameIdMap();
  for (const auto *name : {"PopupFontSize", "PopupOptionHeight"})
    if (const auto it = values.find(name); it != values.end())
      palette.setValue(it->second,
                       visage::theme::ValueId::defaultValue(it->second) *
                           scale);
}
} // namespace drumfoundry::ui
