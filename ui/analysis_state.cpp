#include "analysis_panel.hpp"
#include <cmath>
#include <stdexcept>
namespace drumfoundry::ui {
void AnalysisPanel::LoadView(const editing::Json &analysis) {
  if (!analysis.contains("view"))
    return;
  const auto &v = analysis.at("view");
  const auto read = [&](const char *key, double fallback, double low,
                        double high) {
    const double result = v.value(key, fallback);
    if (!std::isfinite(result) || result < low || result > high)
      throw std::invalid_argument(std::string("Invalid saved view: ") + key);
    return result;
  };
  const double mode = read("comparison", 0, 0, 5);
  if (mode != std::floor(mode))
    throw std::invalid_argument("Invalid comparison view");
  view_.comparison = Comparison(int(mode));
  view_.span = read("span", 8, .02, 60);
  view_.pan = read("pan", 0, -1e6, 1e6);
  view_.split = read("split", .5, .1, .9);
  view_.modelOffset = read("modelOffset", 0, -1e6, 1e6);
  view_.differenceDb = read("differenceDb", 24, 1, 120);
  view_.frequencyLow = read("frequencyLow", 20, 20, 19999);
  view_.frequencyHigh = read("frequencyHigh", 20000, 20, 20000);
  if (view_.frequencyHigh <= view_.frequencyLow)
    throw std::invalid_argument("Invalid frequency zoom range");
  duration_.Set(read("renderSeconds", 8, .25, 60));
  const char *names[]{"Mirror",     "Side by side", "Stacked",
                      "Difference", "Model",        "Reference"};
  comparison_.setText(names[int(mode)]);
}
void AnalysisPanel::PublishState() {
  if (!presentation || hashPending_ || request_.document.is_null())
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
