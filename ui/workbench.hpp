#pragma once
#include "analysis_panel.hpp"
#include "bridge.hpp"
#include "controls.hpp"
#include "decay_hold_panel.hpp"
#include "file_panel.hpp"
#include "help_bubble.hpp"
#include "history_bar.hpp"
#include "live_spectrum.hpp"
#include "meta_panel.hpp"
#include "modal_panel.hpp"
#include "parameter_panel.hpp"
#include "routing_panel.hpp"
#include "split_bar.hpp"

namespace drumfoundry::ui {
// Shared content, independent of CLAP windows and standalone device ownership.
class Workbench : public visage::Frame {
public:
  explicit Workbench(Bridge bridge);
  ~Workbench() override;
  void resized() override;
  void draw(visage::Canvas &) override;
  void Error(const std::string &message);
  bool AnalysisReady() const { return analysis_.Ready(); }
  void OpenRouting();

private:
  void Poll();
  void LayoutRight();
  void SelectPreset();
  void Change(unsigned, double);
  void SetupPanels();
  void SetupFiles();
  void SetupPerformance();
  void SetupRouting();
  float LeftControlsTop() const { return 144 + (routingOpen_ ? 156.f : 0.f); }
  void RefreshDocument();
  void ApplyDocument();
  editing::Json CaptureDocument() const;
  void OpenFitFile(bool save, const editing::Json &);
  void OpenReferenceFile();
  Bridge bridge_;
  visage::EventTimer timer_;
  visage::UiButton preset_{"Kick"}, settings_{"Settings"}, stop_{"Stop"},
      limiter_{"Limiter ON"}, referencePlay_{"Play reference"},
      modelPlay_{"Play render"};
  visage::UiButton fixedStrike_{"Strike"};
  Slider master_{"Master", -60, 0, -12, " dB"};
  Slider hardness_{"Tip hardness", 0, 1, .5};
  Slider velocity_{"Audition velocity", 0, 1, .8};
  Slider spread_{"Gesture spread", 0, 1, .2};
  std::array<visage::UiButton, 3> implements_{{visage::UiButton("Brush"),
                                               visage::UiButton("Mallet"),
                                               visage::UiButton("Stick")}};
  Slider location_{"Strike location", 0, 1, 0};
  Slider mute_{"Mute / closure", 0, 1, 0};
  StrikePad strike_;
  editing::Document document_;
  LiveSpectrum liveSpectrum_; // Outlives the panel that borrows its Frame.
  DecayHoldPanel holdDecay_;
  bool applyingHold_{};
  ParameterPanel excitation_, resonance_;
  visage::ScrollableFrame right_;
  AnalysisPanel analysis_;
  SplitBar analysisSplit_;
  float splitStart_{};
  ModalPanel modal_;
  HistoryBar history_;
  visage::Frame fileShade_;
  FilePanel files_;
  visage::Frame metaShade_;
  MetaPanel meta_;
  visage::UiButton routingToggle_{"▸ Routing"};
  RoutingDiagram routing_;
  visage::Frame routingShade_;
  RoutingPanel routes_;
  bool routingOpen_{};
  HelpBubble help_;
  int documentPreset_{-1};
  unsigned documentRevision_{};
  bool reloadDocument_{};
  editing::Json renderedEvent_;
  unsigned eventDebounce_{};
  std::string error_, status_;
  double reduction_{}, latency_{};
};
} // namespace drumfoundry::ui
