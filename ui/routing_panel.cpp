#include "routing_panel.hpp"
#include <algorithm>
namespace drumfoundry::ui {
namespace {
std::string EndpointLabel(const editing::Document &d,
                          const std::string &endpoint) {
  const auto dot = endpoint.rfind('.');
  const auto id = endpoint.substr(0, dot);
  const auto &root = d.JsonValue();
  const auto &patch =
      root.contains("instrument") ? root.at("instrument") : root;
  for (const auto &node : patch.at("nodes"))
    if (node.at("id") == id)
      return ModuleName(node) + " / " + endpoint.substr(dot + 1);
  return endpoint;
}
} // namespace
RoutingPanel::RoutingPanel() {
  addChild(&diagram_);
  addChild(&scroll_);
  addChild(&close_);
  diagram_.editable = true;
  diagram_.layoutChanged = [this] {
    if (layoutChanged)
      layoutChanged();
  };
  diagram_.error = [this](const auto &text) {
    if (error)
      error(text);
  };
  close_.onToggle() = [this](auto *, bool) { setVisible(false); };
}
void RoutingPanel::Load(editing::Document &d) {
  document_ = &d;
  for (auto &route : routes_)
    scroll_.removeScrolledChild(route.get());
  routes_.clear();
  for (const auto &route : editing::Routes(d)) {
    auto button = std::make_unique<HelpButton>();
    button->help =
        route.required
            ? "Required by this compiled recipe; cannot be disconnected."
            : "Switch this route on or off. There are no hidden route gains. "
              "An edit that disconnects every audible path is rejected.";
    button->setFont(Font());
    button->setActive(!route.required);
    button->setAlphaTransparency(route.required ? .5f : 1.f);
    button->onToggle() = [this, id = route.id](auto *, bool) { Toggle(id); };
    scroll_.addScrolledChild(button.get());
    routes_.push_back(std::move(button));
  }
  Refresh();
  resized();
}
void RoutingPanel::Refresh() {
  if (!document_)
    return;
  const auto routes = editing::Routes(*document_);
  for (unsigned i = 0; i < routes.size(); ++i) {
    const auto &r = routes[i];
    routes_[i]->setText((r.required  ? "Required  |  "
                         : r.enabled ? "ON  |  "
                                     : "OFF  |  ") +
                        EndpointLabel(*document_, r.from) + "  →  " +
                        EndpointLabel(*document_, r.to));
    routes_[i]->setActionButton(r.enabled && !r.required);
  }
  diagram_.Load(*document_);
  redraw();
}
void RoutingPanel::Toggle(const std::string &id) {
  try {
    for (const auto &route : editing::Routes(*document_))
      if (route.id == id) {
        document_->SetRoute(id, !route.enabled);
        break;
      }
    Refresh();
    if (changed)
      changed();
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void RoutingPanel::resized() {
  if (width() < 100 || height() < 100)
    return;
  close_.setBounds(width() - 106, 14, 90, 28);
  diagram_.setBounds(16, 54, width() - 32, 210);
  scroll_.setBounds(16, 304, width() - 32, std::max(1.f, height() - 320));
  for (unsigned i = 0; i < routes_.size(); ++i)
    routes_[i]->setBounds(0, i * 34, scroll_.width() - 14, 30);
  scroll_.setScrollableHeight(float(routes_.size() * 34));
}
void RoutingPanel::draw(visage::Canvas &c) {
  c.setColor(0xff17202a);
  c.roundedRectangle(0, 0, width(), height(), 8);
  Label(c, "INSTRUMENT ROUTING", 16, 14, width() - 140, 28, 0xffe8b755);
  Label(c,
        "Move boxes to arrange the diagram. Switch routes below; levels stay "
        "in the parameter sections.",
        16, 274, width() - 32, 24);
}
} // namespace drumfoundry::ui
