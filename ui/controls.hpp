#pragma once
#include <functional>
#include <memory>
#include <visage/ui.h>
#include <visage/widgets.h>

namespace drumfoundry::ui {
visage::Font Font(float size = 13);
// Typography only: retain Visage's stock widget behaviour and drawing.
void NativeFonts(visage::Frame &);
void ControlErrors(visage::Frame &,
                   const std::function<void(const std::string &)> &);
void Label(visage::Canvas &, const std::string &, float x, float y, float w,
           float h, unsigned color = 0xffcad4df);

// Conventional horizontal slider, using Visage input and drawing primitives.
// Double click resets; Shift-drag gives fine adjustment. Values stay in units.
class Slider : public visage::Frame {
public:
  Slider(std::string label, double low, double high, double initial,
         std::string unit = "");
  void Set(double value);
  void SetDefault(double value);
  // Explicit text entry is strict: invalid/out-of-range input never changes
  // sound.
  bool SubmitText(const std::string &text);
  void resized() override;
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
  void mouseUp(const visage::MouseEvent &) override;
  std::function<void(double)> changed;
  std::function<void()> committed;
  std::function<void(const std::string &)> error;
  bool integer{}; // Count controls reject fractional typed values.
  // Optional unit-preserving taper; the displayed/saved value never changes.
  std::function<double(double)> position, valueAt;

private:
  void BeginText();
  void CloseText();
  void Edit(double value);
  double Position(double value) const;
  double ValueAt(double position) const;
  std::string label_, unit_;
  double low_, high_, initial_, value_, dragValue_{};
  float dragX_{};
  bool dragging_{};
  std::unique_ptr<visage::TextEditor> text_;
};

class StrikePad : public visage::Frame {
public:
  void SetKick(bool kick) {
    if (kick_ != kick) {
      kick_ = kick;
      redraw();
    }
  }
  void SetMembrane(bool membrane) {
    if (membrane_ != membrane) {
      membrane_ = membrane;
      redraw();
    }
  }
  void draw(visage::Canvas &) override;
  void mouseDown(const visage::MouseEvent &) override;
  std::function<void(float, float)> strike;

private:
  bool kick_{};
  bool membrane_{};
};
} // namespace drumfoundry::ui
