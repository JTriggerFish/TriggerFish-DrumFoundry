#pragma once
#include "controls.hpp"
#include <filesystem>
namespace drumfoundry::ui {
// Small in-window browser built from standard Visage widgets. No browser
// permissions, OS shell subprocesses or extra file-dialog runtime dependency.
class FilePanel : public visage::Frame {
public:
  FilePanel();
  void Open(std::filesystem::path directory, bool save,
            std::string extension = ".json");
  void OpenDirectory(std::filesystem::path directory,
                     std::string title = "REFERENCE LIBRARY FOLDER");
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void(const std::filesystem::path &)> chosen;
  std::function<void(const std::string &)> error;

private:
  void Browse();
  void Accept();
  std::filesystem::path Directory() const;
  visage::TextEditor directory_, filename_;
  visage::UiButton browse_{"Browse folder"}, up_{"Up"}, accept_{"Open"},
      close_{"Cancel"};
  bool save_{};
  bool folderOnly_{};
  std::string extension_{".json"};
  std::string folderTitle_;
};
} // namespace drumfoundry::ui
