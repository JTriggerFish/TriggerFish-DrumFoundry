#include "editing/files.hpp"
#include "workbench/decay_hold/coordinates.hpp"
#include "workbench/decay_hold/worker.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace drumfoundry;
using namespace drumfoundry::decay_hold;
namespace {
void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void MeasurementTests() {
  std::vector<float> signal(6 * 48000);
  for (unsigned i = 0; i < signal.size(); ++i)
    signal[i] = float(.25 * std::sin(6.283185307179586 * 750 * i / 48000));
  auto cells = Measure(signal, 48000);
  double power = 0;
  for (unsigned i = 120; i < 144; ++i)
    power += cells[i];
  Check(std::abs(power - .25 * .25 * 3 / 32 * (4095. / 4096)) < 1e-9,
        "Hann positive-frequency power normalization");
  for (float &x : signal)
    x *= 2;
  const auto louder = Measure(signal, 48000);
  for (unsigned i = 0; i < cells.size(); ++i)
    Check(std::abs(louder[i] - 4 * cells[i]) < 1e-12,
          "No level normalization in decay measurement");
  bool cancelled = false;
  try {
    Measure(signal, 48000, [] { return true; });
  } catch (const std::runtime_error &) {
    cancelled = true;
  }
  Check(cancelled, "Measurement cancellation");
}
Cells Model(const Document &d, uint32_t) {
  Cells cells;
  for (unsigned i = 0; i < cells.size(); ++i) {
    const double t60 =
        d.Value(i % 24 < 12 ? "body_decay_seconds_0" : "body_decay_seconds_7");
    cells[i] = i < 120 ? 1
                       : std::pow(10., -6 * (1 + (i / 24 - 5) * .5) /
                                           (t60 * d.Value("bloom_rate")));
  }
  return cells;
}
void SolverTests(Document baseline) {
  baseline.SetMany({{"body_decay_seconds_0", 4},
                    {"body_decay_seconds_7", 4},
                    {"bloom_rate", 1}});
  for (unsigned i = 1; i < 7; ++i)
    baseline.Set("body_decay_active_" + std::to_string(i), 0);
  auto edited = baseline;
  edited.Set("bloom_rate", 1.2);
  Check(Eligible(baseline, edited), "Eligible bloom edit");
  Check(Coordinates(baseline, edited).size() == 3,
        "Only endpoints and unchanged concentration");
  const auto original = edited.JsonValue();
  auto result = Compensate(baseline, edited, 7, Model);
  Check(result.accepted && result.after < .1 && result.evaluations < 40,
        "Known damping compensation");
  Check(edited.JsonValue() == original, "Solver never mutates caller");
  edited.SetMany(result.values);
  Check(std::abs(edited.Value("body_decay_seconds_0") - 4 / 1.2) < .03,
        "Recover known lower T60");
  Check(std::abs(edited.Value("body_decay_seconds_7") - 4 / 1.2) < .03,
        "Recover known upper T60");
  Check(!Eligible(baseline, edited), "Never compensate an explicit T60 edit");
  edited = baseline;
  edited.Set("bloom_rate", 1.2);
  edited.Set("bloom_energy_acceleration", .8);
  Check(Coordinates(baseline, edited).size() == 2,
        "Preserve explicitly edited concentration");
  const auto seedDependent = [](const Document &d, uint32_t seed) {
    auto copy = d;
    if (seed != 7)
      copy.SetMany(
          {{"body_decay_seconds_0", 8 - d.Value("body_decay_seconds_0")},
           {"body_decay_seconds_7", 8 - d.Value("body_decay_seconds_7")}});
    return Model(copy, seed);
  };
  result = Compensate(baseline, edited, 7, seedDependent);
  Check(!result.accepted && result.values.empty(),
        "Reject corrections that only work for the fitting seed");
  result = Compensate(baseline, baseline, 7, Model);
  Check(!result.accepted && result.before == 0 && result.values.empty(),
        "Stable sound unchanged");
  edited.Set("bloom_rate", 4);
  result = Compensate(baseline, edited, 7, Model);
  Check(!result.accepted && result.values.empty(),
        "Reject unreachable target without hidden envelope");
  bool cancelled = false;
  try {
    Compensate(baseline, edited, 7, Model, [] { return true; });
  } catch (const std::runtime_error &) {
    cancelled = true;
  }
  Check(cancelled, "Solver cancellation");
}
} // namespace
int main(int argc, char **argv) {
  Check(argc == 2, "Expected gong preset");
  Document document;
  document.Load(editing::ReadFit(argv[1]));
  MeasurementTests();
  SolverTests(document);
  // Exercise the actual native six-second measurement, without a protected
  // host, Python or a reference file. Silent/nonfinite output is an error.
  const auto rendered = RenderMeasure(document, 1, 48000);
  Check(Rms(Targets(rendered, rendered).Compare(rendered).late) == 0,
        "Native self-comparison");
  Worker worker;
  Check(worker.Start(document.JsonValue(), document.JsonValue(), 48000),
        "Start hold worker");
  worker.Cancel();
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (worker.Busy() && std::chrono::steady_clock::now() < deadline)
    std::this_thread::yield();
  Check(!worker.Busy() && !worker.Take(), "Cancelled completion is discarded");
  std::cout << "Native hold-decay measurement, solver, bounds and cancellation "
               "passed\n";
}
