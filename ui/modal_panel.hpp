#pragma once
#include "modal_plot.hpp"
#include "series_panel.hpp"
namespace drumfoundry::ui {
class ModalPanel : public visage::Frame {
public:
  ModalPanel();
  void Load(editing::Document &);
  void Refresh();
  bool Available() const { return available_; }
  float MinimumHeight();
  std::function<void()> layoutChanged;
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void()> committed;
  std::function<void(const std::string &)> error;

private:
  float LayoutTools();
  void Sync();
  void EditSelection();
  editing::Document *document_{};
  ModalPlot plot_;
  SeriesPanel series_;
  HelpButton tool_{"Select & move"};
  visage::UiButton clear_{"Clear"}, remove_{"Delete mode"},
      generate_{"Generate modes"}, guide_{"Harmonic guide OFF"},
      snap_{"Snap OFF"};
  Slider brush_{"Brush width", .15, 5, 1, " ERB"};
  Slider guidePitch_{"Guide pitch", 8, 8000, 55, " Hz"};
  Slider frequency_{"Centre frequency", 1, 20000, 110, " Hz"};
  Slider level_{"Prominence", -72, 6, 0, " dB"};
  Slider noisiness_{"Local noisiness", 0, 2, 1};
  Slider allocation_{"Sideband allocation", 0, 4, 1};
  bool showSeries_{}, available_{};
};
} // namespace drumfoundry::ui
