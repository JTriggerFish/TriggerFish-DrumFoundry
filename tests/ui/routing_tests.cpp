#include "ui/routing_diagram.hpp"
#include <stdexcept>
void RoutingGestures(drumfoundry::editing::Document d) {
  using namespace drumfoundry;
  ui::RoutingDiagram diagram;
  diagram.editable = true;
  diagram.setBounds(0, 0, 856, 226); // Unit scale for the metallic recipe.
  diagram.Load(d);
  const auto before = editing::NodePositions(d.JsonValue());
  const auto p = before.at("contact");
  unsigned saved = 0;
  diagram.layoutChanged = [&] { ++saved; };
  visage::MouseEvent e;
  e.button_id = visage::kMouseButtonLeft;
  e.position = {p.at("x").get<float>() + 20, p.at("y").get<float>() + 20};
  e.window_position = e.position;
  diagram.mouseDown(e);
  e.window_position.x += 50;
  e.window_position.y += 20;
  diagram.mouseDrag(e);
  diagram.mouseUp(e);
  const auto after = editing::NodePositions(d.JsonValue());
  if (saved != 1 ||
      after.at("contact").at("x") != p.at("x").get<double>() + 50 ||
      after.at("contact").at("y") != p.at("y").get<double>() + 20)
    throw std::runtime_error("Routing diagram drag regression");
}
