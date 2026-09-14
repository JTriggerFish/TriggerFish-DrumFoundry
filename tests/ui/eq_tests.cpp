#include "ui/eq_plot.hpp"
#include "ui/parameter_panel.hpp"
#include <cmath>
#include <complex>
#include <stdexcept>
namespace {
void Check(bool condition) {
  if (!condition)
    throw std::runtime_error("Native EQ response/editor regression");
}
void CheckResponse(const drumfoundry::editing::Document &d) {
  const auto settings = drumfoundry::editing::OutputEqSettings(d);
  for (float rate : {32000.f, 44100.f, 48000.f, 96000.f}) {
    const drumfoundry::editing::OutputEqResponse curve(settings, rate);
    for (double frequency : {60., 1000., 10000.}) {
      tfdsp::percussion::RadiationFilter filter;
      filter.Prepare(rate, settings);
      const auto step = std::polar(1., -6.283185307179586 * frequency / rate);
      std::complex<double> z{1}, response{};
      for (unsigned i = 0; i < 32768; ++i) {
        response += double(filter.Process(i ? 0.f : 1.f)) * z;
        z *= step;
      }
      const auto parts = curve.At(frequency);
      Check(std::abs(20 * std::log10(std::abs(response)) - parts[0] - parts[1] -
                     parts[2]) < .005);
    }
  }
}
} // namespace
void EqTests(drumfoundry::editing::Document d) {
  using namespace drumfoundry;
  Check(editing::HasOutputEq(d));
  for (const auto &p : d.Parameters()) {
    Check(p.key != "equalizer_mode" && p.key.rfind("band_", 0) != 0);
    if (p.key.rfind("output_", 0) == 0)
      Check(editing::Section(p) == "Output" && !editing::RightColumn(p));
    if (d.Recipe() == "drum.kick.v1" &&
        (p.key.rfind("thump_", 0) == 0 || p.key.rfind("tension_", 0) == 0))
      Check(editing::RightColumn(p));
  }
  ui::ParameterPanel panel;
  panel.setBounds(0, 0, 280, 800);
  panel.Load(d, false);
  unsigned plots = 0;
  // ScrollableFrame owns one content frame.
  auto count = [&](auto &&self, visage::Frame &frame) -> void {
    plots += dynamic_cast<ui::EqPlot *>(&frame) != nullptr;
    for (auto *child : frame.children())
      self(self, *child);
  };
  count(count, panel);
  Check(plots == 1);
  d.SetMany({{"output_eq_enabled", 1},
             {"output_low_cut", 60},
             {"output_colour_frequency", 1000},
             {"output_colour_gain", 6},
             {"output_high_cut", 10000}});
  CheckResponse(d);
  const auto before = d.JsonValue();
  ui::EqPlot plot(d, nullptr);
  plot.setBounds(0, 0, 300, 190);
  Check(d.JsonValue() == before);
  unsigned changes = 0, commits = 0;
  plot.changed = [&] { ++changes; };
  plot.committed = [&] { ++commits; };
  const auto point = [](double f, double gain) {
    return visage::Point{float(30 + 262 * std::log(f / 5) / std::log(4400.)),
                         float(26 + 140 * (24 - gain) / 60)};
  };
  visage::MouseEvent e;
  e.button_id = visage::kMouseButtonLeft;
  e.repeat_click_count = 1;
  e.position = point(1000, 6);
  plot.mouseDown(e);
  e.position = point(2000, -3);
  plot.mouseDrag(e);
  plot.mouseUp(e);
  Check(std::abs(d.Value("output_colour_frequency") - 2000) < .002);
  Check(std::abs(d.Value("output_colour_gain") + 3) < .001);
  Check(changes == 1 && commits == 1);
  d.Set("output_eq_enabled", 0);
  const auto bypassed = d.JsonValue();
  plot.mouseDown(e);
  e.position = point(4000, 10);
  plot.mouseDrag(e);
  plot.mouseUp(e);
  Check(d.JsonValue() == bypassed && changes == 1);
  e.position = point(2000, -3);
  e.repeat_click_count = 2;
  plot.mouseDown(e);
  Check(d.Value("output_colour_frequency") ==
        d.Description("output_colour_frequency").initial);
  Check(d.Value("output_colour_gain") ==
        d.Description("output_colour_gain").initial);
}
