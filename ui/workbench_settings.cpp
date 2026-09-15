#include "workbench.hpp"
#include "workbench/analysis/library.hpp"

namespace drumfoundry::ui {
void Workbench::OpenSettings() {
  visage::PopupMenu menu;
  if (bridge_.settings)
    menu.addOption(0, "Audio / MIDI settings…");
  menu.addOption(1, "Reference library folder…");
  menu.addOption(2, "Clear reference library folder");
  menu.addOption(3, "Layout…");
  visage::PopupMenu colours("Colour scheme");
  colours.addOption(5, "Classic (default)");
  colours.addOption(6, "LazyVim");
  colours.addOption(4, "Load JSON…");
  menu.addSubMenu(std::move(colours));
  menu.onSelection() = [this](int item) {
    try {
      if (item == 0 && bridge_.settings)
        bridge_.settings();
      else if (item == 1) {
        files_.chosen = [this](const auto &folder) {
          analysis_.SetLibraryRoot(folder);
        };
        const auto root = analysis_.LibraryRoot();
        files_.OpenDirectory(root.empty() ? std::filesystem::current_path()
                                          : root);
        fileShade_.setVisible(true);
      } else if (item == 2)
        analysis_.SetLibraryRoot({});
      else if (item == 3)
        OpenLayout();
      else if (item == 4) {
        files_.chosen = [this](const auto &path) {
          LoadTheme(ReadTheme(path), true);
        };
        const auto directory = ThemeSettingsPath().parent_path();
        files_.Open(std::filesystem::exists(directory)
                        ? directory
                        : std::filesystem::current_path(),
                    false);
        fileShade_.setVisible(true);
      } else if (item == 5)
        LoadTheme(DefaultTheme(), true);
      else if (item == 6)
        LoadTheme(LazyVimTheme(), true);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  menu.show(&settings_);
}
void Workbench::LoadTheme(const editing::Json &theme, bool persist) {
  ValidateTheme(theme);
  if (persist)
    SaveTheme(theme);
  ApplyTheme(textPalette_, theme);
  NativeFonts(*this); // Invalidate every child, including cached plots.
  if (textSizeChanged)
    textSizeChanged(); // Refresh the standalone settings overlay too.
}
} // namespace drumfoundry::ui
