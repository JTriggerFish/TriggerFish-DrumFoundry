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
  bool Ready() const { return result_ && !worker_.Busy(); }
  unsigned RenderRate() const {
    return result_ && result_->model.sampleRate ? result_->model.sampleRate
                                                : 48000;
  }
  double analysisShare{450. /
                       1100}; // Presentation only; saved with view state.
  void SetAuditionRate(unsigned);
  void Play(bool reference);
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void()> chooseReference;
  std::function<void(const std::string &)> error;
  std::function<void(std::shared_ptr<const std::vector<float>>, unsigned,
                     double)>
      play;
  std::function<void(const editing::Json &, const editing::Json &)>
      presentation;

private:
  void Queue();
  void LoadView(const editing::Json &);
  void PublishState();
  void Menus();
  void ReferenceMenu();
  void SelectReference(const analysis::ReferenceCell &);
  analysis::Catalog catalog_;
  analysis::Worker worker_;
  analysis::Request request_;
  editing::Json reference_;
  editing::Json published_;
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
