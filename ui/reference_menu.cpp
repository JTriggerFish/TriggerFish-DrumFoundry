#include "analysis_panel.hpp"
#include <algorithm>
namespace drumfoundry::ui {
void AnalysisPanel::ReferenceMenu() {
  if (catalog_.Cells().empty()) {
    if (chooseReference)
      chooseReference();
    return;
  }
  visage::PopupMenu menu;
  std::vector<std::string> ids, names;
  for (const auto &cell : catalog_.Cells())
    if (std::find(ids.begin(), ids.end(), cell.corpus) == ids.end()) {
      ids.push_back(cell.corpus);
      names.push_back(cell.corpusName);
    }
  for (unsigned group = 0; group < ids.size(); ++group) {
    visage::PopupMenu cells(names[group]);
    for (unsigned i = 0; i < catalog_.Cells().size(); ++i) {
      const auto &cell = catalog_.Cells()[i];
      if (cell.corpus == ids[group])
        cells.addOption(int(i), cell.metadata.at("label").get<std::string>());
    }
    menu.addSubMenu(std::move(cells));
  }
  menu.addOption(-2, "Other WAV…");
  menu.onSelection() = [this](int i) {
    try {
      if (i == -2) {
        if (chooseReference)
          chooseReference();
      } else if (i >= 0 && unsigned(i) < catalog_.Cells().size())
        SelectReference(catalog_.Cells()[i]);
    } catch (const std::exception &e) {
      if (error)
        error(e.what());
    }
  };
  menu.show(&referenceButton_);
}
void AnalysisPanel::SelectReference(const analysis::ReferenceCell &cell) {
  request_.reference = cell.path;
  request_.expectedHash = cell.metadata.at("sha256");
  matchLength_ = true;
  hashPending_ = true;
  reference_ = {
      {"id", "sha256:" + cell.metadata.at("sha256").get<std::string>()},
      {"sha256", cell.metadata.at("sha256")},
      {"name", cell.metadata.at("label")},
      {"localPath", cell.path.u8string()},
      {"referenceGainDb", cell.gainDb},
      {"corpus", {{"id", cell.corpus}, {"name", cell.corpusName}}},
      {"cell", cell.metadata}};
  referenceGain_.Set(cell.gainDb);
  view_.referenceGainDb = cell.gainDb;
  view_.referenceOffset = cell.metadata.value("onset_seconds", 0.);
  referenceButton_.setText(cell.metadata.at("label").get<std::string>());
  RefreshReferenceControls();
  Queue();
}
} // namespace drumfoundry::ui
