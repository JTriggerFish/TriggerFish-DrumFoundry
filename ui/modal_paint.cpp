#include "editing/curves.hpp"
#include "modal_plot.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
using namespace editing;
void ModalPlot::Paint(visage::Point p, const visage::MouseEvent &e) {
  const double sigma = brush * (e.isCtrlDown()    ? 2.5
                                : e.isShiftDown() ? .35
                                                  : 1);
  const int steps =
      std::max(1, int(std::ceil(std::abs(Erb(Frequency(p.x)) -
                                         Erb(Frequency(previous_.x))) /
                                std::clamp(.65 * brush, .3, 1.25))));
  for (int step = 1; step <= steps; ++step) {
    const float amount = float(step) / steps;
    const double f =
        Snap(Frequency(previous_.x + (p.x - previous_.x) * amount));
    const double centre = Erb(f),
                 target = Level(previous_.y + (p.y - previous_.y) * amount);
    int nearest = -1, free = -1;
    double distance = 1e9;
    for (unsigned i = 0; i < modes_.size(); ++i) {
      if (modes_[i].level <= -72) {
        if (free < 0)
          free = int(i);
        continue;
      }
      const double gap = std::abs(Erb(modes_[i].frequency) - centre);
      if (gap < distance) {
        distance = gap;
        nearest = int(i);
      }
    }
    // Same-direction neighbourhood adjustment: do not raise a valley while
    // lowering the surrounding peaks, which made the old brush frustrating.
    if (nearest >= 0 && distance <= 3 * sigma) {
      const double delta = target - modes_[nearest].level;
      for (auto &m : modes_) {
        const double x = (Erb(m.frequency) - centre) / sigma;
        if (m.level > -72 && std::abs(x) <= 3)
          m.level = std::clamp(m.level + .65 * std::exp(-.5 * x * x) * delta,
                               -72., 6.);
      }
    }
    if (tool == Tool::Paint && distance >= std::clamp(.7 * sigma, .3, 1.4)) {
      if (free < 0) {
        if (error)
          error("All modal handles are in use");
      } else {
        modes_[free] = {f, std::max(-71.9, target), 1, 1};
        selected = free;
      }
    }
  }
  previous_ = p;
  Store();
}
} // namespace drumfoundry::ui
