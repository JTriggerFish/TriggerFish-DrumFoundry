#include "ui/routing_diagram.hpp"
#include <algorithm>
#include <stdexcept>
void RoutingGestures(drumfoundry::editing::Document d) {
  using namespace drumfoundry;
  ui::RoutingDiagram diagram;
  const auto before = editing::NodePositions(d.JsonValue());
  double canvasWidth = 840, canvasHeight = 210;
  for (const auto &p : before) {
    canvasWidth = std::max(canvasWidth, p.at("x").get<double>() + 148);
    canvasHeight = std::max(canvasHeight, p.at("y").get<double>() + 72);
  }
  // Unit scale includes optional attachments below the audio path.
  diagram.setBounds(0, 0, float(canvasWidth + 16), float(canvasHeight + 16));
  diagram.Load(d);
  const auto p = before.at("contact");
  unsigned saved = 0;
  unsigned opened = 0;
  diagram.open = [&] { ++opened; };
  diagram.layoutChanged = [&] { ++saved; };
  visage::MouseEvent e;
  e.button_id = visage::kMouseButtonLeft;
  e.position = {p.at("x").get<float>() + 20, p.at("y").get<float>() + 20};
  e.window_position = e.position;
  e.button_id = visage::kMouseButtonRight;
  diagram.mouseDown(e);
  if (opened != 0)
    throw std::runtime_error("Right-click must not open routing editor");
  e.button_id = visage::kMouseButtonLeft;
  diagram.mouseDown(e);
  diagram.mouseUp(e);
  if (opened != 1 || saved != 0)
    throw std::runtime_error("Single click must open compact routing diagram");
  diagram.editable = true;
  diagram.mouseDown(e);
  e.window_position.x += 50;
  e.window_position.y += 20;
  diagram.mouseDrag(e);
  diagram.mouseUp(e);
  const auto after = editing::NodePositions(d.JsonValue());
  if (saved != 1 || opened != 1 ||
      after.at("contact").at("x") != p.at("x").get<double>() + 50 ||
      after.at("contact").at("y") != p.at("y").get<double>() + 20)
    throw std::runtime_error("Routing diagram drag regression");
}
