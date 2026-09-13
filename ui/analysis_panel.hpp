#pragma once
#include "analysis_view.hpp"
#include "editing/document.hpp"
#include "workbench/analysis/catalog.hpp"
namespace drumfoundry::ui {
class AnalysisPanel : public visage::Frame {
public:
  AnalysisPanel();
  void SetDocument(const editing::Json &);
  void UpdateModel(const editing::Json &document) {
    request_.document = document;
    Queue();
  }
  void SetReference(const std::filesystem::path &);
  editing::Json Reference() const;
  editing::Json Settings() const;
  void Poll();
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void()> chooseReference;
  std::function<void(const std::string &)> error;

private:
  void Queue();
  void Menus();
  void ReferenceMenu();
  void SelectReference(const analysis::ReferenceCell &);
  analysis::Catalog catalog_;
  analysis::Worker worker_;
  analysis::Request request_;
  editing::Json reference_;
  std::shared_ptr<const analysis::Result> result_;
  AnalysisView view_;
  visage::UiButton referenceButton_{"Reference WAV"}, fft_{"FFT 4096"},
      window_{"Hann"}, comparison_{"Mirror"}, reset_{"Reset zoom"},
      channel_{"Mono average"};
  Slider duration_{"Render length", .25, 60, 8, " s"};
  Slider range_{"Colour range", 30, 120, 80, " dB"};
  Slider referenceGain_{"Reference gain", -60, 48, 0, " dB"};
  std::string status_;
  bool hashPending_{}, matchLength_{};
};
} // namespace drumfoundry::ui
