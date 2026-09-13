#include "analysis_panel.hpp"
namespace drumfoundry::ui {
void AnalysisPanel::ReferenceMenu() {
  visage::PopupMenu menu;
  menu.addOption(-1, "None").select(reference_.is_null());
  std::vector<std::filesystem::directory_entry> entries;
  try {
    entries = analysis::LibraryFolder(libraryRoot_, referenceFolder_);
    if (!referenceFolder_.empty())
      menu.addOption(-2, "../ Parent folder");
    for (unsigned i = 0; i < entries.size(); ++i)
      menu.addOption(int(i), (entries[i].is_directory() ? "[folder] " : "") +
                                 entries[i].path().filename().u8string());
  } catch (const std::exception &e) {
    referenceWarning_ = e.what();
    redraw();
  }
  menu.onSelection() = [this, entries](int i) {
    try {
      if (i == -1)
        ClearReference();
      else if (i == -2) {
        referenceFolder_ = std::filesystem::u8path(referenceFolder_)
                               .parent_path()
                               .generic_u8string();
        browsePending_ = true;
      } else if (i >= 0 && unsigned(i) < entries.size()) {
        if (entries[i].is_directory()) {
          referenceFolder_ =
              analysis::LibraryRelativePath(libraryRoot_, entries[i].path());
          browsePending_ = true;
        } else
          SetReference(entries[i].path());
      }
    } catch (const std::exception &e) {
      referenceWarning_ = e.what();
      redraw();
    }
  };
  menu.show(&referenceButton_);
}
} // namespace drumfoundry::ui
