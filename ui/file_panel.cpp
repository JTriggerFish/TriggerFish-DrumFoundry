#include "file_panel.hpp"
#include <algorithm>
namespace drumfoundry::ui {
FilePanel::FilePanel() {
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &directory_, &filename_, &browse_, &up_, &accept_, &close_})
    addChild(frame);
  for (auto *field : {&directory_, &filename_}) {
    field->setMultiLine(false);
    field->setFont(Font());
  }
  filename_.setDefaultText("New fit name.json");
  directory_.onEnterKey() = [this] { Browse(); };
  filename_.onEnterKey() = [this] { Accept(); };
  browse_.onToggle() = [this](auto *, bool) { Browse(); };
  up_.onToggle() = [this](auto *, bool) {
    directory_.setText(Directory().parent_path().u8string());
  };
  accept_.onToggle() = [this](auto *, bool) { Accept(); };
  close_.onToggle() = [this](auto *, bool) { setVisible(false); };
}
void FilePanel::Open(std::filesystem::path directory, bool save,
                     std::string extension) {
  save_ = save;
  extension_ = std::move(extension);
  directory_.setText(directory.u8string());
  filename_.setText("");
  accept_.setText(save ? "Save new fit" : "Open");
  setVisible(true);
}
std::filesystem::path FilePanel::Directory() const {
  return std::filesystem::u8path(directory_.text().toUtf8());
}
void FilePanel::Browse() {
  try {
    std::vector<std::filesystem::directory_entry> entries;
    for (const auto &entry : std::filesystem::directory_iterator(Directory()))
      if (entry.is_directory() || entry.path().extension() == extension_)
        entries.push_back(entry);
    std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
      if (a.is_directory() != b.is_directory())
        return a.is_directory();
      return a.path().filename() < b.path().filename();
    });
    visage::PopupMenu menu;
    menu.addOption(0, "../ parent folder");
    for (unsigned i = 0; i < entries.size(); ++i)
      menu.addOption(int(i + 1),
                     (entries[i].is_directory() ? "[folder] " : "") +
                         entries[i].path().filename().u8string());
    menu.onSelection() = [this, entries](int choice) {
      if (choice == 0)
        directory_.setText(Directory().parent_path().u8string());
      else if (choice > 0 && unsigned(choice) <= entries.size()) {
        const auto &entry = entries[choice - 1];
        if (entry.is_directory())
          directory_.setText(entry.path().u8string());
        else
          filename_.setText(entry.path().filename().u8string());
      }
    };
    menu.show(&browse_);
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void FilePanel::Accept() {
  try {
    if (filename_.text().toUtf8().empty())
      throw std::runtime_error("Enter or choose a filename");
    auto path =
        Directory() / std::filesystem::u8path(filename_.text().toUtf8());
    if (save_ && path.extension().empty())
      path += extension_;
    if (save_ && std::filesystem::exists(path))
      throw std::runtime_error(
          "That file already exists. Use a new name to keep the previous fit.");
    if (chosen)
      chosen(path);
    setVisible(false);
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void FilePanel::resized() {
  directory_.setBounds(18, 58, width() - 36, 32);
  browse_.setBounds(18, 100, 150, 30);
  up_.setBounds(178, 100, 64, 30);
  filename_.setBounds(18, 166, width() - 36, 32);
  accept_.setBounds(width() - 280, height() - 48, 142, 30);
  close_.setBounds(width() - 128, height() - 48, 110, 30);
}
void FilePanel::draw(visage::Canvas &c) {
  c.setColor(0xff18212b);
  c.roundedRectangle(0, 0, width(), height(), 8);
  Label(c, save_ ? "SAVE A NEW FIT" : "OPEN FILE", 18, 12, width() - 36, 28,
        0xffe8b755);
  Label(c, "Folder — paste a path or browse its contents", 18, 36, width() - 36,
        22);
  Label(c, "Filename", 18, 140, width() - 36, 22);
  Label(c,
        save_
            ? "Existing fits are kept; choose a new filename for each version."
            : "Choose a file, then Open.",
        18, 206, width() - 36, 28);
}
} // namespace drumfoundry::ui
