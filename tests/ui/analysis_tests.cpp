#include "editing/files.hpp"
#include "ui/analysis_view.hpp"
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
  analysis::Audio tone{48000, 1, std::vector<float>(48000)};
  for (unsigned i = 0; i < tone.samples.size(); ++i)
    tone.samples[i] =
        .5f * float(std::sin(2 * 3.14159265358979323846 * 750 * i / 48000));
  const auto spectrum = analysis::Analyze(tone, {4096, 512, "hann"});
  Require(std::abs(spectrum.At(.5, 750) + 6.0206) < .001);
  Require(spectrum.At(.5, 3000) < -100 && spectrum.At(-1, 750) == -180);
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
  analysis::Worker worker;
  analysis::Request request;
  request.document = editing::ReadFit(argv[1]);
  ReferenceTests(request.document);
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
