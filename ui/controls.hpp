#pragma once
#include <functional>
#include <visage/ui.h>
#include <visage/widgets.h>

namespace drumfoundry::ui {
visage::Font Font(float size = 13);
void Label(visage::Canvas &, const std::string &, float x, float y, float w,
           float h, unsigned color = 0xffcad4df);

// Conventional horizontal slider, using Visage input and drawing primitives.
// Double click resets; Shift-drag gives fine adjustment. Values stay in units.
class Slider : public visage::Frame {
public:
  Slider(std::string label, double low, double high, double initial,
         std::string unit = "");
  void Set(double value);
  double Value() const { return value_; }
  void SetLabel(std::string label) {
    if (label_ != label) {
      label_ = std::move(label);
      redraw();
    }
  }
  void draw(visage::Canvas &) override;
  void mouseDown(const visage::MouseEvent &) override;
  void mouseDrag(const visage::MouseEvent &) override;
  std::function<void(double)> changed;

private:
  void Edit(double value);
  std::string label_, unit_;
  double low_, high_, initial_, value_, dragValue_{};
  float dragX_{};
};

class StrikePad : public visage::Frame {
public:
  void SetKick(bool kick) {
    if (kick_ != kick) {
      kick_ = kick;
      redraw();
    }
  }
  void draw(visage::Canvas &) override;
  void mouseDown(const visage::MouseEvent &) override;
  std::function<void(float, float)> strike;

private:
  bool kick_{};
};
} // namespace drumfoundry::ui
