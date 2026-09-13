#include "analysis_panel.hpp"
#include <algorithm>
#include <limits>

namespace drumfoundry::ui {
void AnalysisPanel::RefreshReferenceControls() {
  const auto *cell = catalog_.Find(reference_);
  for (auto *button : {&articulation_, &layer_, &take_})
    button->setVisible(cell != nullptr);
  if (cell) {
    referenceButton_.setText("Ref: " + cell->corpusName + " ▾");
    articulation_.setText("Strike: " +
                          cell->metadata.value("articulation", "standard"));
    layer_.setText("Velocity: " +
                   std::to_string(cell->metadata.value("velocity", 0)));
    take_.setText("Take: " + std::to_string(cell->metadata.value("repeat", 1)));
  }
  resized();
}

// Change one reference axis, retaining the other selections where available.
// This selects recorded audio only; it never changes the instrument controls.
void AnalysisPanel::ReferenceDimensionMenu(const char *key,
                                           visage::UiButton &button) {
  const auto *current = catalog_.Find(reference_);
  if (!current)
    return;
  std::vector<editing::Json> values;
  for (const auto &cell : catalog_.Cells()) {
    if (cell.corpus != current->corpus || !cell.metadata.contains(key))
      continue;
    const auto &value = cell.metadata.at(key);
    if (std::find(values.begin(), values.end(), value) == values.end())
      values.push_back(value);
  }
  std::sort(values.begin(), values.end());
  visage::PopupMenu menu;
  for (unsigned i = 0; i < values.size(); ++i)
    menu.addOption(i, values[i].is_string() ? values[i].get<std::string>()
                                            : values[i].dump())
        .select(current->metadata.value(key, editing::Json()) == values[i]);
  menu.onSelection() = [this, axis = std::string(key), values,
                        corpus = current->corpus,
                        previous = current->metadata](int i) {
    try {
      if (i < 0 || unsigned(i) >= values.size())
        return;
      const analysis::ReferenceCell *best = nullptr;
      double score = std::numeric_limits<double>::infinity();
      for (const auto &cell : catalog_.Cells()) {
        if (cell.corpus != corpus ||
            cell.metadata.value(axis, editing::Json()) != values[i])
          continue;
        const double distance =
            (cell.metadata.value("articulation", "") ==
                     previous.value("articulation", "")
                 ? 0
                 : 10000) +
            100 * std::abs(cell.metadata.value("velocity", 0) -
                           previous.value("velocity", 0)) +
            std::abs(cell.metadata.value("repeat", 1) -
                     previous.value("repeat", 1));
        if (distance < score) {
          best = &cell;
          score = distance;
        }
      }
      if (best)
        SelectReference(*best);
    } catch (const std::exception &e) {
      if (error)
        error(e.what());
    }
  };
  menu.show(&button);
}
} // namespace drumfoundry::ui
