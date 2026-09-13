#pragma once
#include "controls.hpp"
#include "editing/modes.hpp"
namespace drumfoundry::ui {
class SeriesPanel : public visage::Frame {
public:
  SeriesPanel();
  void Load(editing::Document &);
  void resized() override;
  void draw(visage::Canvas &) override;
  void Apply();
  std::function<void()> committed;
  std::function<void(const std::string &)> error;

private:
  editing::Series Settings() const;
  void Preview();
  editing::Document *document_{};
  std::vector<std::unique_ptr<Slider>> fields_;
  visage::UiButton family_{"Harmonic"}, note_{"Base note"},
      replace_{"Replace modes"};
  bool membrane_{};
  std::string status_;
};
} // namespace drumfoundry::ui
