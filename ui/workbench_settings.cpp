#include "workbench.hpp"
#include "workbench/analysis/library.hpp"

namespace drumfoundry::ui {
void Workbench::OpenSettings() {
  visage::PopupMenu menu;
  if (bridge_.settings)
    menu.addOption(0, "Audio / MIDI settings…");
  menu.addOption(1, "Reference library folder…");
  menu.addOption(2, "Clear reference library folder");
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
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  menu.show(&settings_);
}
} // namespace drumfoundry::ui
