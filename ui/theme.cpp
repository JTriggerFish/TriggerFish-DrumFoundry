#include "theme.hpp"
#include "theme_json.hpp"
#include <algorithm>
#include <cctype>

namespace drumfoundry::ui {
namespace colours {
#define DF_COLOR(name, value) VISAGE_THEME_COLOR(name, value);
#include "theme_colors.inc"
#undef DF_COLOR
} // namespace colours
namespace {
const std::map<std::string, visage::theme::ColorId> &Tokens() {
  static const std::map<std::string, visage::theme::ColorId> tokens{
#define DF_COLOR(name, value) {#name, colours::name},
#include "theme_colors.inc"
#undef DF_COLOR
  };
  return tokens;
}
unsigned Hex(const std::string &text) {
  const auto rgb = unsigned(std::stoul(text.substr(1, 6), nullptr, 16));
  const auto alpha =
      text.size() == 9 ? unsigned(std::stoul(text.substr(7, 2), nullptr, 16))
                       : 255u;
  return (alpha << 24) | rgb;
}
} // namespace
editing::Json DefaultTheme() {
  return editing::Json::parse(DefaultThemeJson);
}
void ValidateTheme(const editing::Json &j) {
  if (!j.is_object() || j.size() != 3 ||
      j.value("schema", "") != "triggerfish.drumfoundry.theme/v1" ||
      !j.contains("name") || !j.at("name").is_string() ||
      j.at("name").get<std::string>().empty() ||
      j.at("name").get<std::string>().size() > 80 || !j.contains("colors") ||
      !j.at("colors").is_object() || j.at("colors").size() != Tokens().size())
    throw std::runtime_error("Invalid colour scheme: expected name, schema "
                             "and complete colors map");
  for (const auto &[name, id] : Tokens()) {
    const auto &value = j.at("colors").at(name);
    if (!value.is_string())
      throw std::runtime_error("Colour " + name + " must be a hex string");
    const auto text = value.get<std::string>();
    if ((text.size() != 7 && text.size() != 9) || text.front() != '#' ||
        !std::all_of(text.begin() + 1, text.end(),
                     [](unsigned char c) { return std::isxdigit(c); }))
      throw std::runtime_error("Colour " + name +
                               " must be #RRGGBB or #RRGGBBAA");
  }
}
void ApplyTheme(visage::Palette &palette, const editing::Json &j) {
  ValidateTheme(j); // Validate everything before changing any active colour.
  for (const auto &[name, id] : Tokens())
    palette.setColor(id, visage::Color(Hex(j.at("colors").at(name))));
  // Preserve stock Visage interaction/drawing; replace its palette only.
  const auto native = visage::theme::ColorId::nameIdMap();
  const std::pair<const char *, const char *> mappings[]{
      {"UiButtonBackground", "Button"},
      {"UiButtonBackgroundHover", "ButtonHover"},
      {"UiButtonText", "Text"},
      {"UiButtonTextHover", "SelectedText"},
      {"UiActionButtonBackground", "Selected"},
      {"UiActionButtonBackgroundHover", "ButtonHover"},
      {"UiActionButtonText", "SelectedText"},
      {"UiActionButtonTextHover", "SelectedText"},
      {"PopupMenuBackground", "Panel"},
      {"PopupMenuBorder", "Border"},
      {"PopupMenuText", "Text"},
      {"PopupMenuDisabledText", "Muted"},
      {"PopupMenuSelection", "Selected"},
      {"PopupMenuSelectionText", "SelectedText"},
      {"TextEditorBackground", "Raised"},
      {"TextEditorText", "Text"},
      {"TextEditorDefaultText", "Muted"},
      {"TextEditorSelection", "Selected"},
      {"TextEditorCaret", "Accent"},
      {"ScrollBarDefault", "Border"},
      {"ScrollBarDown", "Accent"}};
  for (const auto &[widget, token] : mappings)
    if (const auto found = native.find(widget); found != native.end())
      palette.setColor(found->second,
                       visage::Color(Hex(j.at("colors").at(token))));
}
} // namespace drumfoundry::ui
