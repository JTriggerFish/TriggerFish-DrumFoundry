#include "ui/analysis_view.hpp"
#include "ui/time_axis.hpp"
#include <cmath>
#include <stdexcept>

namespace {
void Check(bool ok) {
  if (!ok)
    throw std::runtime_error("Spectrogram gesture regression");
}
void TimeAxisTests() {
  using namespace drumfoundry;
  for (float width : {300.f, 430.f, 560.f, 1000.f})
    for (float scale : {1.f, 15.f / 13, 17.f / 13})
      for (double split : {0., .1, .37, .5, .9})
        for (bool mirror : {false, true}) {
          auto ticks =
              ui::TimeAxisTicks(42, width, scale, 0, 8, split, mirror);
          unsigned starts = 0;
          std::vector<std::pair<float, float>> labels;
          for (const auto &tick : ticks) {
            starts += tick.seconds == 0;
            Check(tick.labelX >= 42 &&
                  tick.labelX + tick.labelWidth <= 42 + width + .001f);
            labels.emplace_back(tick.labelX, tick.labelX + tick.labelWidth);
            const double u = (tick.x - 42) / width;
            const double local = !split ? u
                                 : u < split - 1e-6
                                     ? u / split
                                     : (u - split) / (1 - split);
            if (!split || std::abs(u - split) > 1e-6)
              Check(std::abs(tick.seconds - 8 * (split && mirror && u < split
                                                     ? 1 - local
                                                     : local)) < 1e-4);
          }
          Check(starts == (split ? 2u : 1u));
          std::sort(labels.begin(), labels.end());
          for (unsigned i = 1; i < labels.size(); ++i)
            Check(labels[i].first >= labels[i - 1].second);
          const auto shifted =
              ui::TimeAxisTicks(42, width, scale, 1.25, 8, split, mirror);
          Check(shifted.size() == ticks.size());
          for (unsigned i = 0; i < ticks.size(); ++i)
            Check(shifted[i].seconds == ticks[i].seconds + 1.25);
        }
}
} // namespace
void AnalysisGestureTests() {
  using namespace drumfoundry;
  TimeAxisTests();
  auto result = std::make_shared<analysis::Result>();
  result->model = {48000, 1, {0.f}};
  result->reference = result->model;
  ui::AnalysisView view;
  view.setBounds(0, 0, 800, 300);
  view.Set(result);
  visage::MouseEvent event;
  event.button_id = visage::kMouseButtonLeft;
  const auto drag = [&](float x, float y, float dx, float dy) {
    event.position = {x, y};
    view.mouseDown(event);
    event.position = {x + dx, y + dy};
    view.mouseDrag(event);
    view.mouseUp(event);
  };
  // Header space is no longer a hidden forward-time waveform drag target.
  drag(200, 10, 30, 0);
  Check(view.pan == 0);
  const double delta = view.span * 30 / (746 * .5);
  drag(200, 40, 30, 0); // Newly reclaimed area belongs to the mirrored plot.
  Check(std::abs(view.pan - delta) < 1e-10);
  drag(230, 40, -30, 0);
  Check(std::abs(view.pan) < 1e-10);
  event.modifiers = visage::kModifierShift;
  drag(200, 40, 30, 0);
  Check(std::abs(view.referenceOffset - delta) < 1e-10 &&
        view.modelOffset == 0);
  drag(600, 40, 30, 0);
  Check(std::abs(view.modelOffset + delta) < 1e-10 &&
        std::abs(view.pan) < 1e-10);
  event.modifiers = 0;
  drag(415, 40, 30, 0);
  Check(std::abs(view.split - (445. - 42) / 746) < 1e-7);
  view.comparison = ui::Comparison::Stacked;
  view.split = .5;
  drag(200, 149.5f, 0, 20);
  Check(std::abs(view.split - 141.5 / 243) < 1e-7);
}
