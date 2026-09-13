#pragma once
#include "controls.hpp"
#include <chrono>
namespace drumfoundry::ui {
// One non-interactive, delayed popup; no retained pointers to rebuilt controls.
class HelpBubble : public visage::Frame {
public:
  HelpBubble();
  void Bind(visage::Frame &);
  void Poll();
  void Hide();
  void draw(visage::Canvas &) override;

private:
  void Queue(const std::string &, visage::Point);
  std::vector<std::string> lines_;
  std::chrono::steady_clock::time_point due_{};
  bool pending_{};
};
} // namespace drumfoundry::ui
