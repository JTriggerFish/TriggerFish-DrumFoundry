#include "controls.hpp"
#include "embedded/fonts.h"
#include <algorithm>
#include <cmath>

namespace drumfoundry::ui {
visage::Font Font(float size) {
  return visage::Font(size, visage::fonts::Lato_Regular_ttf);
}
void NativeFonts(visage::Frame &frame) {
  if (auto *button = dynamic_cast<visage::UiButton *>(&frame))
    button->setFont(FrameFont(frame));
  if (auto *editor = dynamic_cast<visage::TextEditor *>(&frame))
    editor->setFont(FrameFont(frame));
  for (auto *child : frame.children())
    NativeFonts(*child);
  frame.redraw();
}
void ControlErrors(visage::Frame &frame,
                   const std::function<void(const std::string &)> &error) {
  if (auto *slider = dynamic_cast<Slider *>(&frame))
    slider->error = error;
  for (auto *child : frame.children())
    ControlErrors(*child, error);
}
void Label(visage::Canvas &c, const std::string &text, float x, float y,
           float w, float h, visage::theme::ColorId color) {
  c.setColor(color);
  c.text(text, Font(13 * c.value(TextScale)), visage::Font::kLeft, x, y, w,
         h);
}
Slider::Slider(std::string label, double low, double high, double initial,
               std::string unit)
    : label_(std::move(label)), unit_(std::move(unit)), low_(low),
      high_(high), initial_(initial), value_(initial) {
  help =
      label_ + (unit_.empty() ? ". " : " (" + unit_ + "). ") +
      "Double-click resets; Shift-drag adjusts finely. Right-click to type "
      "a value in the displayed units; Enter applies, Escape cancels.";
  if (unit_ == " Hz")
    help += " Type frequencies in Hz, even when the readout shows kHz.";
}
void Slider::Set(double value) {
  if (!std::isfinite(value))
    return;
  value = std::clamp(value, low_, high_);
  if (value_ != value) {
    CloseText(); // An external/preset change invalidates an unfinished entry.
    value_ = value;
    redraw();
  }
}
void Slider::SetDefault(double value) {
  if (std::isfinite(value))
    initial_ = std::clamp(value, low_, high_);
}
void Slider::Edit(double value) {
  Set(value);
  if (changed)
    changed(value_);
}
double Slider::Position(double value) const {
  return position ? position(value) : (value - low_) / (high_ - low_);
}
double Slider::ValueAt(double p) const {
  p = std::clamp(p, 0., 1.);
  return valueAt ? valueAt(p) : low_ + (high_ - low_) * p;
}
void Slider::draw(visage::Canvas &c) {
  const bool stacked = height() >= 62;
  const auto readout = Readout(value_);
  const auto font = FrameFont(*this);
  const float valueWidth =
      font.stringWidth(visage::String(readout).toUtf32());
  const float valueX = stacked ? 0 : std::max(0.f, width() - valueWidth);
  const float captionWidth = stacked ? width() : std::max(0.f, valueX - 8);
  Label(c, ElideText(font, label_, captionWidth), 0, 0, captionWidth, 22,
        colours::Muted);
  Label(c, readout, valueX, stacked ? 22 : 0, width() - valueX, 22);
  const float track = std::max(1.f, width() - 12.f);
  const float trackY = stacked ? 52.f : 30.f;
  c.setColor(colours::Track);
  c.roundedRectangle(6, trackY, track, 4, 2);
  const float x = 6 + track * float(Position(value_));
  c.setColor(colours::Accent);
  c.roundedRectangle(6, trackY, x - 6, 4, 2);
  c.circle(x - 5, trackY - 3, 10);
}
void Slider::mouseDown(const visage::MouseEvent &e) {
  if (e.button_id == visage::kMouseButtonRight) {
    dragging_ = false;
    BeginText();
    return;
  }
  if (!e.isLeftButton())
    return;
  CloseText();
  dragging_ = true;
  if (e.repeatClickCount() == 2)
    Edit(initial_);
  else if (!e.isShiftDown())
    Edit(ValueAt((e.position.x - 6) / std::max(1.f, width() - 12.f)));
  dragX_ = e.position.x;
  dragValue_ = Position(value_);
}
void Slider::mouseDrag(const visage::MouseEvent &e) {
  if (!dragging_)
    return;
  const double fine = e.isShiftDown() ? .1 : 1.;
  Edit(ValueAt(dragValue_ + (e.position.x - dragX_) /
                                std::max(1.f, width() - 12.f) * fine));
}
void Slider::mouseUp(const visage::MouseEvent &e) {
  const bool commit = dragging_ && e.isLeftButton();
  dragging_ = false;
  if (commit && committed)
    committed();
}
} // namespace drumfoundry::ui
