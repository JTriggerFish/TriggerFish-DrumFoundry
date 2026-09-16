#include "editing/files.hpp"
#include "ui/analysis_panel.hpp"
#include "ui/analysis_view.hpp"
#include "ui/frequency_axis.hpp"
#include "ui/preview_tracker.hpp"
#include "workbench/analysis/worker.hpp"
#include <chrono>
#include <cmath>
#include <stdexcept>
using namespace drumfoundry;
void ReferenceTests(const Json &);
void Require(bool condition) {
  if (!condition)
    throw std::runtime_error("Native analysis regression");
}
int main(int argc, char **argv) {
  Require(argc == 2);
  for (double topHz : {20000., 16000., 12750.}) {
    for (float h : {100.f, 400.f}) {
      const auto ticks = ui::FrequencyAxisTicks(20, topHz, 20, h, 16);
      Require(!ticks.empty() && ticks.front().frequency == topHz &&
              ticks.front().labelY == 20);
      for (unsigned i = 0; i < ticks.size(); ++i) {
        Require(ticks[i].labelY >= 20 && ticks[i].labelY + 16 <= 20 + h);
        if (i)
          Require(ticks[i].labelY >= ticks[i - 1].labelY + 18);
      }
    }
  }
  analysis::Audio tone{48000, 1, std::vector<float>(48000)};
  for (unsigned i = 0; i < tone.samples.size(); ++i)
    tone.samples[i] =
        .5f * float(std::sin(2 * 3.14159265358979323846 * 750 * i / 48000));
  const auto spectrum = analysis::Analyze(tone, {4096, 512, "hann"});
  Require(std::abs(spectrum.At(.5, 750) + 6.0206) < .001);
  Require(spectrum.At(.5, 3000) < -100 && spectrum.At(-1, 750) == -180);
  Require(std::abs(spectrum.Peak(.45, .55, 700, 800) + 6.0206) < .001);
  Require(spectrum.Peak(-.1, -.01, 700, 800) == -180);
  Require(spectrum.Peak(1.2, 1.3, 700, 800) == -180);
  analysis::Spectrogram sparse{48000, 2048, 512,
                               8,     1025, std::vector<float>(8 * 1025, -180)};
  sparse.db[4 * 1025 + 777] = -12;
  Require(sparse.At(.04, 18000) == -180);
  Require(sparse.Peak(.02, .06, 17000, 19000) == -12);
  Require(analysis::Analyze(tone, {4096, 512, "hann"}, [] {
            return true;
          }).frames == 0);
  Require(analysis::Resample(tone, 48000) == tone.samples);
  const auto converted = analysis::Resample(tone, 44100);
  Require(std::abs(double(converted.size()) - 44100) <= 1);
  double squaredError = 0;
  for (unsigned i = 1000; i < 40000; ++i) {
    const double expected =
        .5 * std::sin(2 * 3.14159265358979323846 * 750 * i / 44100);
    squaredError += std::pow(converted[i] - expected, 2);
  }
  Require(std::sqrt(squaredError / 39000) < 1e-5);
  ui::AnalysisView view;
  view.setBounds(0, 0, 800, 300);
  visage::MouseEvent e;
  e.precise_wheel_delta_y = 1;
  view.mouseWheel(e);
  e.precise_wheel_delta_y = -1;
  view.mouseWheel(e);
  Require(std::abs(view.pan) < 1e-10);
  e.button_id = visage::kMouseButtonLeft;
  e.position = {200, 40}; // The plot now begins immediately below its legend.
  view.mouseDown(e);
  e.position.x += 30;
  view.mouseDrag(e);
  view.mouseUp(e);
  Require(std::abs(view.pan + view.span * 30 / (800 - 54)) < 1e-10);
  view.pan = 0;
  extern void AnalysisGestureTests();
  AnalysisGestureTests();
  analysis::Worker worker;
  analysis::Request request;
  request.document = editing::ReadFit(argv[1]);
  ui::PreviewTracker tracker;
  tracker.Reset(request.document);
  Require(!tracker.Advance(request.document));
  auto dragged = request.document;
  dragged["controls"]["event"]["strength"] = .123;
  Require(!tracker.Advance(dragged));
  for (unsigned i = 0; i < 3; ++i)
    Require(!tracker.Advance(dragged));
  Require(tracker.Advance(dragged));
  Require(!tracker.Advance(dragged));
  dragged["instrument"]["nodes"][0]["editor"]["x"] = 89;
  for (unsigned i = 0; i < 5; ++i)
    Require(!tracker.Advance(dragged));
  {
    auto document = request.document;
    document["reference"] = nullptr;
    document["controls"]["analysis"]["view"] = {{"comparison", 3},
                                                {"pan", .25},
                                                {"span", 2},
                                                {"split", .4},
                                                {"modelOffset", .003},
                                                {"differenceDb", 12},
                                                {"renderSeconds", .25},
                                                {"frequencyLow", 100},
                                                {"frequencyHigh", 5000},
                                                {"analysisShare", 450. / 1100},
                                                {"leftShare", .4},
                                                {"showSpectrogram", 1},
                                                {"showModalEditor", 1},
                                                {"singleColumn", 0},
                                                {"textSize", 2}};
    ui::AnalysisPanel panel;
    panel.SetDocument(document);
    Require(panel.Settings().at("view") ==
            document.at("controls").at("analysis").at("view"));
    const auto previous = panel.Settings();
    for (double invalid : {-1., .5, 3.}) {
      auto badText = document;
      badText["controls"]["analysis"]["view"]["textSize"] = invalid;
      bool rejected = false;
      try {
        panel.SetDocument(badText);
      } catch (const std::exception &) {
        rejected = true;
      }
      Require(rejected && panel.Settings() == previous);
    }
    document["controls"]["analysis"]["view"]["span"] = 12;
    document["controls"]["analysis"]["view"]["frequencyHigh"] = 25;
    bool rejected = false;
    try {
      panel.SetDocument(document);
    } catch (const std::exception &) {
      rejected = true;
    }
    Require(rejected && panel.Settings() == previous);
  }
  ReferenceTests(request.document);
  extern void LibraryTests(Json);
  LibraryTests(request.document);
  extern void LiveSpectrumTests();
  LiveSpectrumTests();
  extern void StreamingTests();
  StreamingTests();
  extern void LivePanelTests(Json);
  LivePanelTests(request.document);
  request.duration = .25;
  request.transform = {512, 128, "hann"};
  worker.Submit(request);
  std::shared_ptr<const analysis::Result> result;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (!(result = worker.Take()) &&
         std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  Require(result && result->error.empty() &&
          result->model.samples.size() == 12000);
  Voice voice(48000, request.document);
  voice.Trigger(voice.Event());
  std::vector<float> expected(12000);
  voice.Process(expected.data(), expected.size());
  Require(expected ==
          result->model.samples); // Worker runs the same native DSP.
}
