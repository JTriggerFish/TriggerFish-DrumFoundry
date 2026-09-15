#include "help_bubble.hpp"
#include <algorithm>
#include <sstream>
namespace drumfoundry::ui {
HelpBubble::HelpBubble() {
  setOnTop(true);
  setIgnoresMouseEvents(true, false);
  setVisible(false);
}
void HelpBubble::Bind(visage::Frame &frame) {
  if (auto *hint = dynamic_cast<HelpText *>(&frame);
      hint && !hint->helpBound) {
    hint->helpBound = true;
    // Append, never replace native hover/press behaviour.
    // The callback belongs to the control, so its help remains alive. Read
    // it on hover: status/error readouts can change after binding.
    frame.onMouseEnter() += [this, hint](const auto &event) {
      Queue(hint->help, event.windowPosition());
    };
    frame.onMouseExit() += [this](const auto &) { Hide(); };
    frame.onMouseDown() += [this](const auto &) { Hide(); };
  }
  for (auto *child : frame.children())
    Bind(*child);
}
void HelpBubble::Queue(const std::string &text, visage::Point point) {
  Hide();
  if (text.empty() || !parent())
    return;
  const float w = std::min(400.f, parent()->width() - 20);
  if (w < 80)
    return;
  lines_.clear();
  std::istringstream words(text);
  std::string word, line;
  while (words >> word) {
    const auto next = line.empty() ? word : line + " " + word;
    if (!line.empty() && FrameFont(*this).stringWidth(
                             visage::String(next).toUtf32()) > w - 24) {
      lines_.push_back(line);
      line = word;
    } else
      line = next;
  }
  if (!line.empty())
    lines_.push_back(line);
  const float h = float(lines_.size()) * 18 * paletteValue(TextScale) + 20;
  const float y = point.y + 22 + h <= parent()->height() - 10
                      ? point.y + 22
                      : point.y - h - 12;
  setBounds(std::clamp(point.x, 10.f, parent()->width() - w - 10),
            std::max(10.f, y), w, h);
  due_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
  pending_ = true;
}
void HelpBubble::Poll() {
  if (pending_ && std::chrono::steady_clock::now() >= due_) {
    pending_ = false;
    setVisible(true);
  }
}
void HelpBubble::Hide() {
  pending_ = false;
  setVisible(false);
}
void HelpBubble::draw(visage::Canvas &c) {
  c.setColor(colours::Border);
  c.roundedRectangle(0, 0, width(), height(), 5);
  c.setColor(colours::Panel);
  c.roundedRectangle(1, 1, width() - 2, height() - 2, 4);
  for (unsigned i = 0; i < lines_.size(); ++i)
    Label(c, lines_[i], 12, 10 + i * 18 * paletteValue(TextScale),
          width() - 24, 18 * paletteValue(TextScale));
}
} // namespace drumfoundry::ui
