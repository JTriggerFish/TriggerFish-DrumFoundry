#include "workbench.hpp"
namespace drumfoundry::ui {
void Workbench::SetupRouting() {
  addChild(&routingToggle_);
  addChild(&routing_, false);
  addChild(&routingShade_, false);
  routingShade_.setOnTop(true);
  routingShade_.addChild(&routes_);
  routingShade_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(0x8005090f);
    c.fill(0, 0, width(), height());
  };
  routingToggle_.onToggle() = [this](auto *, bool) {
    routingOpen_ = !routingOpen_;
    routingToggle_.setText(routingOpen_
                               ? "▾ Routing — double-click diagram to edit"
                               : "▸ Routing");
    routing_.setVisible(routingOpen_);
    resized();
    redraw();
  };
  routing_.open = [this] { OpenRouting(); };
  routes_.onVisibilityChange() = [this] {
    if (!routes_.isVisible())
      routingShade_.setVisible(false);
  };
  routes_.changed = [this] {
    ApplyDocument();
    routing_.Load(document_);
  };
  routes_.layoutChanged = [this] {
    try {
      if (bridge_.layout)
        bridge_.layout(editing::NodePositions(document_.JsonValue()));
      routing_.Load(document_);
    } catch (const std::exception &e) {
      Error(e.what());
    }
  };
  routing_.error = routes_.error = [this](const auto &text) { Error(text); };
}
void Workbench::OpenRouting() {
  routes_.Load(document_);
  routes_.setVisible(true);
  routingShade_.setVisible(true);
  NativeFonts(routes_);
  help_.Bind(routes_);
}
} // namespace drumfoundry::ui
