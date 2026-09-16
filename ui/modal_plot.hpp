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
  bool IsSelected(unsigned slot) const;
  unsigned SelectionCount() const;
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
  int Hit(visage::Point, bool handlesOnly = false) const;
  void Paint(visage::Point, const visage::MouseEvent &);
  void Store();
  void SelectOnly(int slot);
  void BeginSelection(const visage::MouseEvent &, int hit);
  void SelectRectangle(visage::Point);
  void MoveSelection(const visage::MouseEvent &);
  void DrawSelection(visage::Canvas &);
  visage::Bounds SelectionBounds() const;
  bool SelectionContains(visage::Point) const;
  editing::Document *document_{};
  std::vector<editing::Mode> modes_;
  std::vector<bool> selection_, selectionBefore_;
  std::vector<editing::Mode> dragModes_;
  bool dragging_{}, marquee_{}, edited_{};
  float widthMovement_{};
  visage::Point previous_, origin_, corner_, movement_;
};
} // namespace drumfoundry::ui
