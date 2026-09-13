#include "ui/decay_editor.hpp"
#include "ui/modal_plot.hpp"
#include <fstream>
#include <stdexcept>
using namespace drumfoundry;
void Require(bool value) {
  if (!value)
    throw std::runtime_error("Modal gesture regression");
}
int main(int argc, char **argv) {
  Require(argc == 2);
  std::ifstream input(argv[1]);
  editing::Document d;
  d.Load(editing::Json::parse(input));
  extern void EqTests(editing::Document);
  EqTests(d);
  extern void RoutingGestures(editing::Document);
  RoutingGestures(d);
  extern void DecayHoldPolicy(editing::Document);
  DecayHoldPolicy(d);
  editing::ReplaceModes(d, {});
  ui::ModalPlot plot;
  plot.setBounds(0, 0, 800, 300);
  plot.Load(d);
  unsigned commits = 0, errors = 0;
  plot.committed = [&] { ++commits; };
  plot.error = [&](const auto &) { ++errors; };
  visage::MouseEvent e;
  e.button_id = visage::kMouseButtonLeft;
  e.position = {300, 120};
  e.repeat_click_count = 2;
  plot.mouseDown(e);
  Require(editing::Modes(d)[0].level > -72 && commits == 1);
  plot.mouseDown(e);
  Require(editing::Modes(d)[0].level == -72 && commits == 2);
  plot.tool = ui::ModalPlot::Tool::Paint;
  e.repeat_click_count = 1;
  plot.mouseDown(e);
  e.position.x += 200;
  plot.mouseDrag(e);
  plot.mouseUp(e);
  unsigned active = 0;
  for (auto m : editing::Modes(d))
    active += m.level > -72;
  Require(active > 1 && active <= 32 && errors == 0);
  const auto before = d.JsonValue();
  ui::DecayEditor decay(d);
  decay.setBounds(0, 0, 300, 366);
  Require(d.JsonValue() == before); // Merely opening editors must not retune.
}
