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
    auto directory = UserPresetDirectory();
    std::filesystem::create_directories(directory);
    files_.chosen = [this, save,
                     document](const std::filesystem::path &path) {
      if (save)
        editing::WriteNewFit(path, document);
      else {
        const auto source = editing::ReadFit(path);
        const auto imported =
            NewPreset(source, source.value("name", path.stem().u8string()));
        const auto destination =
            UserPresetDirectory() /
            (imported.at("id").get<std::string>() + ".json");
        editing::WriteNewFit(destination, imported);
        bridge_.applyDocument(imported);
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
