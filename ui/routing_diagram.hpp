#pragma once
#include "controls.hpp"
#include "editing/routes.hpp"
namespace drumfoundry::ui {
class RoutingDiagram : public visage::Frame, public HelpText {
public:
  RoutingDiagram();
  void Load(editing::Document &);
  void draw(visage::Canvas &) override;
  void mouseDown(const visage::MouseEvent &) override;
  void mouseDrag(const visage::MouseEvent &) override;
  void mouseUp(const visage::MouseEvent &) override;
  bool editable{};
  std::function<void()> open, layoutChanged;
  std::function<void(const std::string &)> error;

private:
  visage::Point Map(visage::Point) const;
  visage::Point Port(const std::string &, bool output) const;
  void Fit();
  editing::Document *document_{};
  editing::Json positions_;
  std::vector<editing::Route> routes_;
  double canvasWidth_{840}, canvasHeight_{210};
  float Scale() const;
  std::string dragging_;
  visage::Point origin_, nodeOrigin_;
};
std::string ModuleName(const editing::Json &node);
visage::theme::ColorId ModuleColour(const std::string &type);
} // namespace drumfoundry::ui
