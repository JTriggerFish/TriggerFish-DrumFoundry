#pragma once
#include "editing/document.hpp"
#include <filesystem>
#include <visage/ui.h>

namespace drumfoundry::ui {
namespace colours {
#define DF_COLOR(name, value) extern const visage::theme::ColorId name;
#include "theme_colors.inc"
#undef DF_COLOR
} // namespace colours

// Complete semantic palette, separate from sound/preset state.
editing::Json DefaultTheme();
void ValidateTheme(const editing::Json &);
void ApplyTheme(visage::Palette &, const editing::Json &);
editing::Json ReadTheme(const std::filesystem::path &);
std::filesystem::path ThemeSettingsPath();
// Atomic local preference update; never modifies the imported theme file.
void SaveTheme(const editing::Json &,
               const std::filesystem::path & = ThemeSettingsPath());
} // namespace drumfoundry::ui
