#include "controls.hpp"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace drumfoundry::ui {
namespace {
std::string Number(double value) {
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::setprecision(12) << value;
  return stream.str();
}
} // namespace
bool Slider::SubmitText(const std::string &text) {
  std::istringstream stream(text);
  stream.imbue(std::locale::classic());
  double value{};
  const bool parsed = bool(stream >> value);
  stream >> std::ws;
  if (!parsed || !stream.eof() || !std::isfinite(value) || value < low_ ||
      value > high_ || (integer && value != std::round(value))) {
    if (error)
      error(
          label_ +
          (integer ? ": enter an integer from " : ": enter a number from ") +
          Number(low_) + " to " + Number(high_) + unit_ +
          ". Enter applies; Escape cancels.");
    return false;
  }
  // Hide before notifying: a commit is allowed to rebuild/delete this
  // control.
  CloseText();
  Edit(value);
  if (committed)
    committed();
  return true;
}
void Slider::BeginText() {
  if (!text_) {
    text_ = std::make_unique<visage::TextEditor>();
    text_->setMultiLine(false);
    text_->setFont(FrameFont(*this));
    text_->setSelectOnFocus(true);
    text_->onEnterKey() = [this] { SubmitText(text_->text().toUtf8()); };
    text_->onEscapeKey() = [this] { CloseText(); };
    addChild(text_.get());
  }
  text_->setText(
      Number(value_)); // Always physical units, e.g. Hz, never kHz.
  text_->setVisible(true);
  resized();
  text_->requestKeyboardFocus();
  text_->selectAll();
}
void Slider::CloseText() {
  if (text_)
    text_->setVisible(false);
}
void Slider::resized() {
  if (text_)
    text_->setBounds(width() * .48f, 0, width() * .52f, 24);
}
} // namespace drumfoundry::ui
