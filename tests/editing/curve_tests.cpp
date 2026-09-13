#include "editing/curves.hpp"
#include "editing/meta.hpp"
#include "editing/modes.hpp"
#include <cmath>
#include <fstream>
#include <stdexcept>
using namespace drumfoundry::editing;
void Require(bool value) {
  if (!value)
    throw std::runtime_error("Native curve regression");
}
int main(int argc, char **argv) {
  Require(argc == 2);
  std::ifstream input(argv[1]);
  Document d;
  d.Load(Json::parse(input));
  auto timing = BloomTiming(d, 0);
  for (const auto &[key, value] : timing.values)
    Require(value == d.Value(key));
  timing = BloomTiming(d, 1);
  Require(std::abs(timing.values[0].second - d.Value("bloom_rate") * .5) <
          1e-12);
  Require(std::abs(timing.values[1].second - (d.Value("body_brightness") - 6)) <
          1e-12);
  const auto neutral = SizeMeta(d, .5);
  for (const auto &[key, value] : neutral.values)
    Require(std::abs(value - d.Description(key).initial) < 1e-10);
  for (double at : {0., .25, .75, 1.}) {
    auto sized = d;
    sized.SetMany(SizeMeta(d, at).values);
    Document checked;
    checked.Load(sized.JsonValue());
    Require(checked.JsonValue() == sized.JsonValue());
  }
  for (double s : {.02, .1, 1., 5., 15., 30.})
    Require(std::abs(DecaySeconds(DecayPosition(s)) - s) < 1e-10);
  for (int i = 1; i < 7; ++i)
    DeleteDecay(d, i);
  Require(DecayKnots(d).size() == 2);
  SetDecay(d, 0, 1, 1);
  SetDecay(d, 7, 1, 4);
  const double middle = InverseErb((Erb(40) + Erb(15000)) / 2);
  Require(std::abs(DecayAt(d, middle) - 2) < 1e-10);
  ShiftDecay(d, 1);
  Require(std::abs(DecayAt(d, middle) - 4) < 1e-10);
  const auto slot = InsertDecay(d, 1000, 3);
  Require(DecayKnots(d).size() == 3);
  DeleteDecay(d, slot);
  Require(DecayKnots(d).size() == 2);
  Series s;
  s.fundamental = 55;
  s.stretch = .5;
  const auto modes = GenerateSeries(s, 8, 15000, 32);
  Require(modes.size() == 16);
  for (int i = 0; i < 4; ++i)
    Require(modes[i].frequency == 55 * (i + 1));
  s.count = 20;
  const auto more = GenerateSeries(s, 8, 15000, 32);
  for (int i = 0; i < 16; ++i)
    Require(modes[i].frequency == more[i].frequency);
  s.family = SeriesFamily::Membrane;
  s.stretch = 0;
  Require(std::abs(GenerateSeries(s, 8, 15000, 32)[1].frequency -
                   55 * 1.593340506) < 1e-10);
  ReplaceModes(d, modes);
  Require(Modes(d)[0].frequency == 55 && Modes(d)[16].level == -72);
  const auto before = d.JsonValue();
  bool rejected = false;
  try {
    SetMode(d, 0, {55, 100, 1, 1});
  } catch (...) {
    rejected = true;
  }
  Require(rejected && d.JsonValue() == before);
  ReplaceModes(d, {});
  Require(InsertMode(d, 100, -6) == 0);
  Require(Modes(d)[0].frequency == 100);
  Document validated;
  validated.Load(d.JsonValue());
  Require(validated.JsonValue() == d.JsonValue());
}
