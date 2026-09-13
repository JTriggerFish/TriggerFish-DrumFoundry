#pragma once
#include "controls.hpp"
#include "editing/meta.hpp"
namespace drumfoundry::ui {
class MetaPanel : public visage::Frame {
public:
  MetaPanel();
  void Open(editing::Document &, bool size);
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void()> changed, committed;
  std::function<void(const std::string &)> error;

private:
  void Preview();
  editing::Document *document_{};
  editing::Document baseline_;
  bool size_{};
  Slider amount_{"Bloom timing", 0, 1, .5};
  visage::UiButton keep_{"Keep"}, cancel_{"Cancel"};
  std::string status_;
};
} // namespace drumfoundry::ui
