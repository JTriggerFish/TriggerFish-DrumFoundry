#include "eq_plot.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
namespace {
constexpr const char *keys[]{"output_low_cut", "output_colour_frequency",
                             "output_high_cut"};
}
int EqPlot::Hit(visage::Point point) const {
  int best = -1;
  double distance = 14;
  for (int i = 0; i < 3; ++i) {
    const double d = std::hypot(
        point.x - X(document_.Value(keys[i])),
        point.y - Y(i == 1 ? document_.Value("output_colour_gain") : 0));
    if (d < distance) {
      best = i;
      distance = d;
    }
  }
  return best;
}
void EqPlot::ResetHandle(int i) {
  std::vector<std::pair<std::string, double>> values{
      {keys[i], document_.Description(keys[i]).initial}};
  if (i == 1)
    values.push_back({"output_colour_gain",
                      document_.Description("output_colour_gain").initial});
  document_.SetMany(values);
  if (changed)
    changed();
  redraw();
}
void EqPlot::mouseDown(const visage::MouseEvent &e) {
  drag_ = -1;
  if (!e.isLeftButton())
    return;
  const int i = Hit(e.position);
  if (i < 0)
    return;
  try {
    if (e.repeatClickCount() == 2) {
      ResetHandle(i);
      if (committed)
        committed();
    } else if (document_.Value("output_eq_enabled") >= .5)
      drag_ = i;
  } catch (const std::exception &ex) {
    if (error)
      error(ex.what());
  }
}
void EqPlot::mouseDrag(const visage::MouseEvent &e) {
  if (drag_ < 0)
    return;
  try {
    const auto &p = document_.Description(keys[drag_]);
    std::vector<std::pair<std::string, double>> values{
        {p.key, std::clamp(Frequency(e.position.x), p.minimum, p.maximum)}};
    if (drag_ == 1) {
      const auto &gain = document_.Description("output_colour_gain");
      values.push_back({gain.key, std::clamp(Gain(e.position.y), gain.minimum,
                                             gain.maximum)});
    }
    document_.SetMany(values);
    if (changed)
      changed();
    redraw();
  } catch (const std::exception &ex) {
    if (error)
      error(ex.what());
  }
}
void EqPlot::mouseUp(const visage::MouseEvent &) {
  const bool commit = drag_ >= 0;
  drag_ = -1;
  if (commit && committed)
    committed();
}
} // namespace drumfoundry::ui
