#pragma once
#include "controls.hpp"
#include "editing/modes.hpp"

namespace drumfoundry::ui {
class ModalPlot : public visage::Frame {
public:
  enum class Tool { Edit, Shape, Paint };
  void Load(editing::Document &document);
  void Refresh();
  void draw(visage::Canvas &) override;
  void mouseDown(const visage::MouseEvent &) override;
  void mouseDrag(const visage::MouseEvent &) override;
  void mouseUp(const visage::MouseEvent &) override;
  bool mouseWheel(const visage::MouseEvent &) override;
  bool keyPress(const visage::KeyEvent &) override;
  void Remove();
  void SnapAll();
  int selected{-1};
  Tool tool{Tool::Edit};
  double brush{1}, base{55};
  bool guide{}, snap{};
  std::function<void()> changed, committed;
  std::function<void(const std::string &)> error;

private:
  float X(double) const;
  float Y(double) const;
  double Frequency(float) const;
  double Level(float) const;
  double Snap(double) const;
  double Spread(const editing::Mode &) const;
  double PacketFrequency(const editing::Mode &, double offset) const;
  int Hit(visage::Point) const;
  void Paint(visage::Point, const visage::MouseEvent &);
  void Store();
  editing::Document *document_{};
  std::vector<editing::Mode> modes_;
  bool dragging_{};
  visage::Point previous_;
};
} // namespace drumfoundry::ui
