#include "routing_diagram.hpp"
#include <algorithm>
#include <cmath>
#include <map>
namespace drumfoundry::ui {
std::string ModuleName(const editing::Json &node) {
  static const std::map<std::string, std::string> names{
      {"exciter.contact", "Contact"},
      {"exciter.thump", "Thump"},
      {"exciter.correlated-fm", "Correlated FM"},
      {"body.stochastic-modal-field", "Metallic body"},
      {"body.membrane-modal", "Modal membrane"},
      {"interaction.strike-energy", "Strike energy"},
      {"interaction.wire-rack", "Snare wires"},
      {"interaction.rim-contact", "Rim contact"},
      {"observation.equalizer", "Output colour"},
      {"observation.dual-source", "Observation"},
      {"observation.three-source", "Observation"},
      {"transform.sum2", "Two-source mix"},
      {"transform.sum3", "Source mix"},
      {"output.mono", "Mono"}};
  const auto type = node.at("type").get<std::string>();
  if (type == "interaction.rim-contact" &&
      node.at("parameters").at("hat_contact_enabled").get<double>() < .5)
    return "Rim contact · off";
  const auto found = names.find(type);
  return node.value("name", found == names.end() ? type : found->second);
}
visage::theme::ColorId ModuleColour(const std::string &type) {
  const auto role = type.substr(0, type.find('.'));
  if (role == "exciter")
    return colours::Warning;
  if (role == "body")
    return colours::Accent;
  if (role == "interaction")
    return colours::Success;
  if (role == "observation")
    return colours::Secondary;
  if (role == "transform")
    return colours::Secondary;
  return colours::Muted;
}
RoutingDiagram::RoutingDiagram() {
  help = "Click for routing controls. Optional routes can be switched "
         "on/off; required connections stay locked. The expanded diagram's "
         "boxes can be moved without changing the sound.";
}
void RoutingDiagram::Load(editing::Document &d) {
  document_ = &d;
  dragging_.clear();
  positions_ = editing::NodePositions(d.JsonValue());
  routes_ = editing::Routes(d);
  Fit();
  redraw();
}
void RoutingDiagram::Fit() {
  canvasWidth_ = 840;
  canvasHeight_ = 210;
  for (const auto &p : positions_) {
    const double x = p.at("x"), y = p.at("y");
    if (!std::isfinite(x) || !std::isfinite(y) || x < 0 || x > 4096 ||
        y < 0 || y > 2048)
      throw std::invalid_argument("Invalid saved routing layout");
    canvasWidth_ = std::max(canvasWidth_, x + 148);
    canvasHeight_ = std::max(canvasHeight_, y + 72);
  }
}
float RoutingDiagram::Scale() const {
  return float(std::max(.01, std::min((width() - 16) / canvasWidth_,
                                      (height() - 16) / canvasHeight_)));
}
visage::Point RoutingDiagram::Map(visage::Point p) const {
  const auto s = Scale();
  return {8 + p.x * s, (height() - float(canvasHeight_) * s) / 2 + p.y * s};
}
visage::Point RoutingDiagram::Port(const std::string &endpoint,
                                   bool output) const {
  const auto id = endpoint.substr(0, endpoint.rfind('.'));
  std::vector<std::string> ports;
  for (const auto &route : routes_) {
    const auto &key = output ? route.from : route.to;
    if (key.substr(0, key.rfind('.')) == id &&
        std::find(ports.begin(), ports.end(), key) == ports.end())
      ports.push_back(key);
  }
  const auto i =
      std::find(ports.begin(), ports.end(), endpoint) - ports.begin();
  const auto &p = positions_.at(id);
  return Map({p.at("x").get<float>() + (output ? 124.f : 0.f),
              p.at("y").get<float>() +
                  48.f * float(i + 1) / float(ports.size() + 1)});
}
void RoutingDiagram::draw(visage::Canvas &c) {
  if (width() < 32 || height() < 32)
    return;
  c.setColor(colours::Background);
  c.roundedRectangle(0, 0, width(), height(), 5);
  if (!document_)
    return;
  const auto &d = document_->JsonValue();
  const auto &nodes =
      (d.contains("instrument") ? d.at("instrument") : d).at("nodes");
  for (const auto &route : routes_) {
    const auto a = Port(route.from, true), b = Port(route.to, false);
    const float bend = std::max(16.f, std::abs(b.x - a.x) * .4f);
    visage::Path path;
    path.moveTo(a);
    path.bezierTo({a.x + bend, a.y}, {b.x - bend, b.y}, b);
    c.setColor(!route.enabled ? colours::Grid :
               route.interaction ? colours::Success : colours::Border);
    c.fill(path.stroke(route.enabled ? 1.5f : 1.f));
  }
  const float scale = Scale();
  for (const auto &node : nodes) {
    const auto &p = positions_.at(node.at("id").get<std::string>());
    const auto point = Map({p.at("x"), p.at("y")});
    const auto type = node.at("type").get<std::string>();
    c.setColor(ModuleColour(type));
    c.roundedRectangle(point.x, point.y, 124 * scale, 48 * scale, 4);
    c.setColor(colours::Panel);
    c.roundedRectangle(point.x + 1, point.y + 1, 124 * scale - 2,
                       48 * scale - 2, 3);
    c.setColor(colours::Text);
    c.text(ModuleName(node), FrameFont(*this, 13 * scale),
           visage::Font::kLeft, point.x + 6 * scale, point.y + 4 * scale,
           112 * scale, 22 * scale);
    c.setColor(ModuleColour(type));
    c.text(type.substr(0, type.find('.')), FrameFont(*this, 11 * scale),
           visage::Font::kLeft, point.x + 6 * scale, point.y + 26 * scale,
           112 * scale, 16 * scale);
  }
}
} // namespace drumfoundry::ui
