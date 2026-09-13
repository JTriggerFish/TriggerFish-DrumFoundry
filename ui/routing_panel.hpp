#pragma once
#include "routing_diagram.hpp"
#include <visage_ui/scroll_bar.h>
namespace drumfoundry::ui {
class RoutingPanel : public visage::Frame {
public:
  RoutingPanel();
  void Load(editing::Document &);
  void Refresh();
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void()> changed, layoutChanged;
  std::function<void(const std::string &)> error;

private:
  void Toggle(const std::string &);
  editing::Document *document_{};
  RoutingDiagram diagram_;
  visage::ScrollableFrame scroll_;
  visage::UiButton close_{"Close"};
  std::vector<std::unique_ptr<HelpButton>> routes_;
};
} // namespace drumfoundry::ui
