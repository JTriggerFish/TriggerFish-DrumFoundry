#include "analysis_panel.hpp"
#include "analysis_validation.hpp"
#include <cmath>
#include <stdexcept>
namespace drumfoundry::ui {
void AnalysisPanel::LoadView(const editing::Json &analysis) {
  const auto v = ReadSavedView(analysis);
  if (v.is_null())
    return;
  const int mode = v.at("comparison").get<int>();
  view_.comparison = Comparison(int(mode));
  view_.span = v.at("span");
  view_.pan = v.at("pan");
  view_.split = v.at("split");
  view_.modelOffset = v.at("modelOffset");
  view_.differenceDb = v.at("differenceDb");
  view_.frequencyLow = v.at("frequencyLow");
  view_.frequencyHigh = v.at("frequencyHigh");
  duration_.Set(v.at("renderSeconds"));
  analysisShare = v.at("analysisShare");
  const char *names[]{"Mirror",     "Side by side", "Stacked",
                      "Difference", "Model",        "Reference"};
  comparison_.setText(names[int(mode)]);
}
void AnalysisPanel::PublishState() {
  if (!presentation || request_.document.is_null())
    return;
  const auto ref = Reference(), settings = Settings();
  const editing::Json next = {{"reference", ref}, {"analysis", settings}};
  if (next == published_)
    return;
  try {
    presentation(ref, settings);
    published_ = next;
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
} // namespace drumfoundry::ui
