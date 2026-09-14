#include "ui/eq_plot.hpp"
#include "ui/parameter_panel.hpp"
#include <cmath>
#include <stdexcept>

namespace {
void CheckAt(bool ok, int line) {
  if (!ok)
    throw std::runtime_error("Grouped EQ input regression at line " +
                             std::to_string(line));
}
#define Check(condition) CheckAt(condition, __LINE__)
template <class T>
T *Find(visage::Frame &frame, const std::string &help = "") {
  if (auto *item = dynamic_cast<T *>(&frame))
    if (help.empty() || item->help.rfind(help, 0) == 0)
      return item;
  for (auto *child : frame.children())
    if (auto *item = Find<T>(*child, help))
      return item;
  return nullptr;
}
visage::Point Point(const drumfoundry::ui::EqPlot &plot, double hz,
                    double db) {
  return {
      float(30 + (plot.width() - 38) * std::log(hz / 5) / std::log(4400.)),
      float(26 + (plot.height() - 50) * (24 - db) / 60)};
}
} // namespace

void EqPanelTests(drumfoundry::editing::Document d) {
  using namespace drumfoundry::ui;
  visage::Palette palette;
  ParameterPanel panel;
  panel.setPalette(&palette);
  unsigned commits = 0;
  panel.committed = [&] { ++commits; };
  for (int size : {0, 1, 2}) {
    ConfigureTextSize(palette, size);
    panel.setBounds(0, 0, 320, 600);
    d.SetMany({{"output_eq_enabled", 0},
               {"output_colour_frequency", 1000},
               {"output_colour_gain", 6}});
    panel.Load(d, false);
    auto *plot = Find<EqPlot>(panel);
    auto *enabled =
        Find<HelpButton>(panel, ParameterHelp("output_eq_enabled"));
    auto *gain = Find<Slider>(panel, ParameterHelp("output_colour_gain"));
    Check(plot && enabled && gain);
    for (float scroll : {0.f, 30.f}) {
      panel.setYPosition(scroll);
      visage::MouseEvent event;
      event.button_id = visage::kMouseButtonLeft;
      event.position = Point(*plot, d.Value("output_colour_frequency"),
                             d.Value("output_colour_gain"));
      const auto inPanel = event.position + plot->positionInWindow() -
                           panel.positionInWindow();
      Check(panel.frameAtPoint(inPanel) ==
            plot); // Card padding/scroll cannot intercept it.
      plot->processMouseDown(event);
      event.position = Point(*plot, 1800 + scroll, -3);
      plot->processMouseDrag(event);
      plot->processMouseUp(event);
      Check(std::abs(d.Value("output_colour_frequency") - 1800 - scroll) <
            .01);
      Check(std::abs(gain->Value() + 3) < .001);
      Check(d.Value("output_eq_enabled") ==
            0); // Editing never enables audio implicitly.
    }
    enabled->onToggle().callback(enabled, false);
    Check(d.Value("output_eq_enabled") == 1); // One click, no buried menu.
    Check(gain->SubmitText("4"));
    Check(d.Value("output_colour_gain") == 4);
    enabled->onToggle().callback(enabled, false);
    Check(d.Value("output_eq_enabled") == 0);
  }
  Check(commits >= 12);
}
