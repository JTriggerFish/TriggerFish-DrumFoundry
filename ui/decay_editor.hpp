#pragma once
#include "controls.hpp"
#include "editing/curves.hpp"
namespace drumfoundry::ui {
class DecayEditor : public visage::Frame {
public:
  explicit DecayEditor(editing::Document &);
  void resized() override;
  void draw(visage::Canvas &) override;
  void mouseDown(const visage::MouseEvent &) override;
  void mouseDrag(const visage::MouseEvent &) override;
  void mouseUp(const visage::MouseEvent &) override;
  std::function<void()> committed;
  std::function<void(const std::string &)> error;

private:
  void Sync();
  void Remove();
  float X(double) const;
  float Y(double) const;
  double Frequency(float) const;
  double Seconds(float) const;
  int Hit(visage::Point) const;
  editing::Document &document_;
  int selected_{}, drag_{-1};
  visage::Point previous_;
  Slider seconds_{"T60", .02, 30, 1, " s"};
  Slider frequency_{"Frequency", 40, 15000, 40, " Hz"};
  visage::UiButton remove_{"Delete knot"};
};
} // namespace drumfoundry::ui
