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
      float(32 + (plot.height() - 137) * (24 - db) / 60)};
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
    if (d.Recipe() == "metal.cymbal.v1") {
      for (const auto *key : {"contact_noise_level", "contact_noise_decay",
                              "contact_noise_colour", "body_decay_friction",
                              "hat_openness", "hat_clearance", "hat_contact_loss",
                              "hat_pedal_strength", "hat_rattle_motion", "hat_settling"})
        Check(Find<Slider>(panel, ParameterHelp(key)) != nullptr);
    }
    auto *plot = Find<EqPlot>(panel);
    auto *enabled =
        Find<HelpButton>(panel, ParameterHelp("output_eq_enabled"));
    auto *gain = Find<Slider>(panel, ParameterHelp("output_colour_gain"));
    Check(plot && enabled && !gain); // No duplicate EQ sliders.
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
      Check(std::abs(d.Value("output_colour_gain") + 3) < .001);
      Check(d.Value("output_eq_enabled") ==
            0); // Editing never enables audio implicitly.
    }
    enabled->onToggle().callback(enabled, false);
    Check(d.Value("output_eq_enabled") == 1); // One click, no buried menu.
    Check(plot->SubmitValue(2, "4"));
    Check(d.Value("output_colour_gain") == 4);
    for (const auto *bad : {"nan", "inf", "garbage", "4dB", "9999"})
      Check(!plot->SubmitValue(2, bad));
    Check(d.Value("output_colour_gain") == 4);
    Check(plot->SubmitValue(0, "60"));
    Check(plot->SubmitValue(1, "1700"));
    Check(plot->SubmitValue(3, "9000"));
    Check(plot->SubmitValue(4, "0.1")); // Decimal DSP endpoint is legal.
    Check(plot->SubmitValue(4, "20"));
    Check(plot->SubmitValue(4, "3.5"));
    Check(d.Value("output_colour_q") == 3.5);
    for (const auto *bad : {"0", "21", "nan", "3.5Q"})
      Check(!plot->SubmitValue(4, bad));
    visage::MouseEvent wheel;
    wheel.position = Point(*plot, 1700, 4);
    wheel.precise_wheel_delta_y = 1;
    Check(plot->mouseWheel(wheel));
    Check(d.Value("output_colour_q") > 3.5);
    wheel.precise_wheel_delta_y = -1;
    Check(plot->mouseWheel(wheel));
    Check(std::abs(d.Value("output_colour_q") - 3.5) < 1e-6);
    wheel.position = Point(*plot, 60, 0);
    Check(!plot->mouseWheel(wheel)); // Other wheel gestures keep scrolling.
    auto *readout =
        Find<HelpButton>(*plot, ParameterHelp("output_colour_gain"));
    Check(readout);
    readout->onToggle().callback(readout, false);
    visage::TextEditor *entry = nullptr;
    for (auto *child : plot->children())
      if (auto *text = dynamic_cast<visage::TextEditor *>(child))
        entry = text;
    Check(entry && entry->isVisible());
    entry->setText("3.25");
    entry->onEnterKey().callback();
    Check(!entry->isVisible() && d.Value("output_colour_gain") == 3.25);
    readout->onToggle().callback(readout, false);
    entry->setText("9");
    entry->onEscapeKey().callback();
    Check(!entry->isVisible() && d.Value("output_colour_gain") == 3.25);
    for (auto *child : plot->children())
      if (child->isVisible())
        Check(child->x() >= 0 && child->y() >= 0 &&
              child->right() <= plot->width() &&
              child->bottom() <= plot->height());
    enabled->onToggle().callback(enabled, false);
    Check(d.Value("output_eq_enabled") == 0);
  }
  Check(commits >= 12);
}
