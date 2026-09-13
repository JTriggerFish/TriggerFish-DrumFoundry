#pragma once
#include "bridge.hpp"
#include "controls.hpp"
#include "parameter_panel.hpp"

namespace drumfoundry::ui {
// Shared content, independent of CLAP windows and standalone device ownership.
class Workbench : public visage::Frame {
public:
  explicit Workbench(Bridge bridge);
  ~Workbench() override;
  void resized() override;
  void draw(visage::Canvas &) override;
  void Error(const std::string &message);

private:
  void Poll();
  void SelectPreset();
  void Change(unsigned, double);
  void RefreshDocument();
  void ApplyDocument();
  Bridge bridge_;
  visage::EventTimer timer_;
  visage::UiButton preset_{"Kick"}, settings_{"Settings"}, stop_{"Stop"},
      limiter_{"Limiter ON"};
  Slider master_{"Master", -60, 0, -12, " dB"};
  Slider hardness_{"Tip hardness", 0, 1, .5};
  std::array<visage::UiButton, 3> implements_{{visage::UiButton("Brush"),
                                               visage::UiButton("Mallet"),
                                               visage::UiButton("Stick")}};
  Slider location_{"Strike location", 0, 1, 0};
  Slider mute_{"Mute / closure", 0, 1, 0};
  StrikePad strike_;
  editing::Document document_;
  ParameterPanel excitation_, resonance_;
  int documentPreset_{-1};
  unsigned documentRevision_{};
  bool reloadDocument_{};
  std::string error_, status_;
  double reduction_{}, latency_{};
};
} // namespace drumfoundry::ui
