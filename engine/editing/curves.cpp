#include "curves.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace drumfoundry::editing {
double Erb(double frequency) {
  return 21.4 * std::log10(1 + .00437 * frequency);
}
double InverseErb(double rate) {
  return (std::pow(10., rate / 21.4) - 1) / .00437;
}
// Display-only one-second knee; DSP still interpolates log T60 in ERB space.
double DecayPosition(double seconds) {
  return (std::log1p(seconds) - std::log1p(.02)) /
         (std::log1p(30) - std::log1p(.02));
}
double DecaySeconds(double position) {
  return std::expm1(std::log1p(.02) + std::clamp(position, 0., 1.) *
                                          (std::log1p(30) - std::log1p(.02)));
}
std::vector<DecayKnot> DecayKnots(const Document &d) {
  std::vector<DecayKnot> result;
  for (int i = 0; i < 8; ++i) {
    if (i > 0 && i < 7 &&
        d.Value("body_decay_active_" + std::to_string(i)) < .5)
      continue;
    result.push_back(
        {i,
         i == 0   ? 40
         : i == 7 ? 15000
                  : d.Value("body_decay_frequency_" + std::to_string(i)),
         d.Value("body_decay_seconds_" + std::to_string(i)), i == 0 || i == 7});
  }
  std::sort(result.begin(), result.end(),
            [](auto a, auto b) { return a.frequency < b.frequency; });
  return result;
}
double DecayAt(const Document &d, double frequency) {
  const auto points = DecayKnots(d);
  const double x = Erb(std::clamp(frequency, 40., 15000.));
  for (std::size_t i = 1; i < points.size(); ++i) {
    if (frequency > points[i].frequency && i + 1 < points.size())
      continue;
    const auto &a = points[i - 1], &b = points[i];
    const double span = Erb(b.frequency) - Erb(a.frequency);
    const double t =
        span > 0 ? std::clamp((x - Erb(a.frequency)) / span, 0., 1.) : 0;
    return std::exp(std::log(a.seconds) + t * std::log(b.seconds / a.seconds));
  }
  return points.back().seconds;
}
void SetDecay(Document &d, int slot, double frequency, double seconds) {
  const auto points = DecayKnots(d);
  const auto it = std::find_if(points.begin(), points.end(),
                               [slot](auto p) { return p.slot == slot; });
  if (it == points.end())
    throw std::invalid_argument("Select an active decay knot");
  if (!std::isfinite(frequency) || !std::isfinite(seconds))
    throw std::invalid_argument("Invalid decay point");
  if (!it->boundary) {
    const double low = Erb((it - 1)->frequency) + .22;
    const double high = Erb((it + 1)->frequency) - .22;
    frequency =
        InverseErb(std::clamp(Erb(frequency), low, std::max(low, high)));
    d.Set("body_decay_frequency_" + std::to_string(slot), frequency);
  }
  d.Set("body_decay_seconds_" + std::to_string(slot),
        std::clamp(seconds, .02, 30.));
}
int InsertDecay(Document &d, double frequency, double seconds) {
  if (!std::isfinite(frequency) || !std::isfinite(seconds) || frequency <= 40 ||
      frequency >= 15000)
    throw std::invalid_argument("Add decay knots between 40 Hz and 15 kHz");
  for (const auto &point : DecayKnots(d))
    if (std::abs(Erb(frequency) - Erb(point.frequency)) < .22)
      throw std::invalid_argument(
          "Decay knot is too close to an existing knot");
  for (int i = 1; i < 7; ++i) {
    const auto suffix = std::to_string(i);
    if (d.Value("body_decay_active_" + suffix) >= .5)
      continue;
    d.Set("body_decay_frequency_" + suffix, frequency);
    d.Set("body_decay_seconds_" + suffix, std::clamp(seconds, .02, 30.));
    d.Set("body_decay_active_" + suffix, 1);
    return i;
  }
  throw std::invalid_argument("All eight decay knots are in use");
}
void DeleteDecay(Document &d, int slot) {
  if (slot <= 0 || slot >= 7)
    throw std::invalid_argument("Boundary knots cannot be deleted");
  d.Set("body_decay_active_" + std::to_string(slot), 0);
}
void ShiftDecay(Document &d, double octaves) {
  if (!std::isfinite(octaves))
    throw std::invalid_argument("Invalid decay shift");
  const auto points = DecayKnots(d);
  double low = -100, high = 100;
  for (auto p : points) {
    low = std::max(low, std::log2(.02 / p.seconds));
    high = std::min(high, std::log2(30 / p.seconds));
  }
  const double gain = std::exp2(std::clamp(octaves, low, high));
  for (auto p : points)
    d.Set("body_decay_seconds_" + std::to_string(p.slot),
          std::clamp(p.seconds * gain, .02, 30.));
}
} // namespace drumfoundry::editing
