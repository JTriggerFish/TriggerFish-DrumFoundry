#include "history_bar.hpp"
#include <algorithm>
#include <chrono>
#include <random>
namespace drumfoundry::ui {
HistoryBar::HistoryBar() {
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &name_, &snapshot_, &history_, &save_, &load_})
    addChild(frame);
  name_.setMultiLine(false);
  name_.setFont(Font());
  name_.setDefaultText("Snapshot name");
  snapshot_.onToggle() = [this](auto *, bool) { Snapshot(); };
  history_.onToggle() = [this](auto *, bool) { SelectSnapshot(); };
  save_.onToggle() = [this](auto *, bool) {
    try {
      if (file)
        file(true, NamedDocument());
    } catch (const std::exception &e) {
      if (error)
        error(e.what());
    }
  };
  load_.onToggle() = [this](auto *, bool) {
    if (file)
      file(false, nullptr);
  };
}
editing::Json HistoryBar::NamedDocument() {
  auto document = capture();
  const auto name = name_.text().toUtf8();
  if (!name.empty())
    document["name"] = name;
  const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  document["parentId"] = document.value("id", "");
  document["id"] = "native-" + std::to_string(stamp) + "-" +
                   std::to_string(std::random_device{}());
  return document;
}
void HistoryBar::Snapshot() {
  try {
    const auto directory = editing::FitDirectory();
    std::filesystem::create_directories(directory);
    const auto document = NamedDocument();
    editing::WriteNewFit(
        directory / (document.at("id").get<std::string>() + ".json"), document);
    history_.setText("Snapshot saved");
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void HistoryBar::SelectSnapshot() {
  try {
    const auto directory = editing::FitDirectory();
    std::vector<std::filesystem::path> paths;
    if (std::filesystem::exists(directory))
      for (const auto &entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json")
          paths.push_back(entry.path());
    std::sort(paths.rbegin(), paths.rend());
    if (paths.empty())
      throw std::runtime_error(
          "No snapshots yet. Name the sound, then press Snapshot.");
    visage::PopupMenu menu;
    for (unsigned i = 0; i < paths.size(); ++i) {
      std::string label = paths[i].stem().u8string();
      try {
        label = editing::FitName(paths[i]);
      } catch (...) {
        label += " (invalid file)";
      } // Still selectable; load reports the actual error.
      menu.addOption(int(i), label + "  ·  " + paths[i].stem().u8string());
    }
    menu.onSelection() = [this, paths](int i) {
      try {
        if (i >= 0 && unsigned(i) < paths.size())
          restore(editing::ReadFit(paths[i]));
      } catch (const std::exception &e) {
        if (error)
          error(e.what());
      }
    };
    menu.show(&history_);
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void HistoryBar::resized() {
  name_.setBounds(0, 0, 220, 30);
  snapshot_.setBounds(228, 0, 100, 30);
  history_.setBounds(336, 0, 150, 30);
  save_.setBounds(494, 0, 90, 30);
  load_.setBounds(592, 0, 90, 30);
}
} // namespace drumfoundry::ui
