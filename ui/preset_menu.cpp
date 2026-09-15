#include "preset_panel.hpp"
#include <set>

namespace drumfoundry::ui {
void AddPresetFolders(visage::PopupMenu &menu,
                      const std::filesystem::path &root,
                      const std::vector<std::filesystem::path> &paths) {
  std::set<std::filesystem::path> folders;
  for (const auto &path : paths) {
    const auto relative = path.lexically_relative(root);
    if (!relative.empty() && *relative.begin() != ".." &&
        relative.has_parent_path())
      folders.insert(*relative.begin());
  }
  for (const auto &folder : folders) {
    visage::PopupMenu child(folder.u8string());
    AddPresetFolders(child, root / folder, paths);
    menu.addSubMenu(std::move(child));
  }
  for (unsigned i = 0; i < paths.size(); ++i) {
    if (paths[i].parent_path() != root)
      continue;
    std::string label;
    try {
      label = PresetLabel(paths[i]);
    } catch (...) {
      label = paths[i].stem().u8string() + " (unreadable)";
    }
    menu.addOption(1000 + int(i), label);
  }
}
} // namespace drumfoundry::ui
