#include "workbench.hpp"
namespace drumfoundry::ui {
editing::Json Workbench::CaptureDocument() const {
  auto next = document_.JsonValue();
  next["controls"]["event"] = bridge_.document().at("controls").at("event");
  next["reference"] = analysis_.Reference();
  next["controls"]["analysis"] = analysis_.Settings();
  return next;
}
void Workbench::OpenFitFile(bool save, const editing::Json &document) {
  try {
    auto directory = editing::FitDirectory();
    std::filesystem::create_directories(directory);
    files_.chosen = [this, save, document](const auto &path) {
      if (save)
        editing::WriteNewFit(path, document);
      else {
        bridge_.applyDocument(editing::ReadFit(path));
        reloadDocument_ = true;
      }
    };
    files_.Open(directory, save);
    fileShade_.setVisible(true);
  } catch (const std::exception &e) {
    Error(e.what());
  }
}
} // namespace drumfoundry::ui
