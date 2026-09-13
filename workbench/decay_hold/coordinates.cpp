#include "coordinates.hpp"
#include "runtime/voice.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace drumfoundry::decay_hold {
double Axis::Decode(double x) const {
  return std::clamp(logarithmic ? std::exp(x) : x, minimum, maximum);
}
std::vector<Axis> Coordinates(const Document &baseline,
                              const Document &edited) {
  std::vector<Axis> result;
  for (const auto &p : edited.Parameters()) {
    const bool log = p.key.find("body_decay_seconds_") == 0;
    if (log) {
      const int index = p.key.back() - '0';
      if (index != 0 && index != 7 &&
          edited.Value("body_decay_active_" + std::to_string(index)) < .5)
        continue;
    } else if (p.key != "bloom_energy_acceleration" ||
               baseline.Value(p.key) != edited.Value(p.key))
      continue;
    const auto encode = [log](double x) { return log ? std::log(x) : x; };
    const double origin = encode(edited.Value(p.key)),
                 reach = log ? std::log(2.) : .12;
    result.push_back(
        {p.key, origin, std::max(encode(p.minimum), origin - reach),
         std::min(encode(p.maximum), origin + reach), log ? .03 : .015,
         log ? .35 : .06, p.minimum, p.maximum, log});
  }
  return result;
}
bool Eligible(const Document &baseline, const Document &edited) {
  if (baseline.Recipe() != "metal.cymbal.v1" ||
      baseline.Recipe() != edited.Recipe())
    return false;
  static const std::set<std::string> keys{
      "bloom_rate", "bloom_energy_acceleration", "bloom_energy_sensitivity",
      "body_brightness", "body_excitation_centre"};
  bool changed = false;
  for (const auto &p : edited.Parameters()) {
    if (baseline.Value(p.key) == edited.Value(p.key))
      continue;
    if (!keys.count(p.key))
      return false;
    changed = true;
  }
  return changed;
}
Cells RenderMeasure(const Document &document, uint32_t seed, unsigned rate,
                    const Cancel &cancel) {
  CheckCancelled(cancel);
  if (rate < 8000 || rate > 384000)
    throw std::invalid_argument("Invalid hold-decay sample rate");
  Voice voice(float(rate), document.JsonValue());
  auto event = voice.Event();
  event.seed = seed;
  voice.Trigger(event);
  std::vector<float> samples(6u * rate);
  for (std::size_t i = 0; i < samples.size(); i += 2048) {
    CheckCancelled(cancel);
    voice.Process(samples.data() + i,
                  std::min(std::size_t(2048), samples.size() - i));
  }
  return Measure(samples, rate, cancel);
}
} // namespace drumfoundry::decay_hold
