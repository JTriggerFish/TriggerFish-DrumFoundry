#include "modes.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace drumfoundry::editing {
namespace {
// Lowest distinct circular-membrane Bessel roots / j_(0,1). The protected-core
// stretch below is shared by native editing and fitting, not a physical gong
// law.
constexpr double Membrane[]{1,           1.593340506, 2.135548787, 2.295417267,
                            2.653066405, 2.917295455, 3.155464815, 3.500147490,
                            3.598484674, 3.647451179, 4.058931883, 4.131738160,
                            4.230439128, 4.601044534, 4.610051645, 4.831885263,
                            4.903280573, 5.083567174, 5.130768907, 5.412118430,
                            5.540398510, 5.553126477, 5.650842377, 5.976540222,
                            6.019355807, 6.152609172, 6.163136731, 6.208732131,
                            6.482735446, 6.528612452, 6.668996901, 6.746213300};

double StretchFactor(unsigned ordinal, unsigned core, double stretch) {
  const double upper = std::max(0., (double(ordinal) - core) / core);
  const double factor = std::hypot(1., stretch * upper);
  return stretch < 0 ? 1. / factor : factor;
}
} // namespace
std::vector<Mode> GenerateSeries(const Series &s, double minimum,
                                 double maximum, unsigned capacity) {
  for (double v : {s.fundamental, s.stretch, s.level, s.rolloff, s.turbulence,
                   minimum, maximum})
    if (!std::isfinite(v))
      throw std::invalid_argument("Series values must be finite");
  if (s.fundamental < minimum || minimum <= 0 || maximum < minimum ||
      s.count < 1 || s.count > std::min(32u, capacity) || s.harmonicCore < 1 ||
      s.harmonicCore > 8 || s.stretch < 0 || s.stretch > 1 ||
      s.turbulence < 0 || s.turbulence > 2 || s.first < 1 || s.first > 32 ||
      (s.family != SeriesFamily::Harmonic &&
       s.family != SeriesFamily::Membrane) ||
      (s.family == SeriesFamily::Membrane && s.first + s.count - 1 > 32))
    throw std::invalid_argument("Invalid modal series settings");
  std::vector<Mode> result;
  for (unsigned i = 0; i < s.count; ++i) {
    const unsigned ordinal = s.first + i;
    const double base =
        s.family == SeriesFamily::Membrane ? Membrane[ordinal - 1] : ordinal;
    const double ratio =
        base * StretchFactor(ordinal, s.harmonicCore, s.stretch);
    if (s.fundamental * ratio > maximum) {
      if (s.truncateToRange && !result.empty())
        break;
      throw std::invalid_argument("Only " + std::to_string(result.size()) +
                                  " modes fit this pitch/stretch range");
    }
    result.push_back(
        {s.fundamental * ratio,
         std::clamp(s.level - s.rolloff * std::log2(ratio), -72., 6.),
         s.turbulence, 1});
  }
  return result;
}

std::optional<std::vector<double>>
TransformSeries(const std::vector<double> &frequencies, double pitch,
                double stretch, unsigned core, double minimum, double maximum) {
  for (double v : {pitch, stretch, minimum, maximum})
    if (!std::isfinite(v))
      throw std::invalid_argument("Series transform values must be finite");
  if (pitch <= 0 || std::abs(stretch) > 1 || core < 1 || core > 8 ||
      minimum <= 0 || maximum < minimum || frequencies.size() > 32)
    throw std::invalid_argument("Invalid series transform settings");
  for (double frequency : frequencies)
    if (!std::isfinite(frequency) || frequency < minimum || frequency > maximum)
      throw std::invalid_argument("Invalid source mode frequency");

  std::vector<unsigned> order(frequencies.size());
  std::iota(order.begin(), order.end(), 0u);
  std::stable_sort(order.begin(), order.end(), [&](unsigned a, unsigned b) {
    return frequencies[a] < frequencies[b];
  });
  auto result = frequencies;
  for (unsigned rank = 0; rank < order.size(); ++rank) {
    const unsigned slot = order[rank];
    const double frequency =
        frequencies[slot] * pitch * StretchFactor(rank + 1, core, stretch);
    if (!std::isfinite(frequency) || frequency < minimum || frequency > maximum)
      return std::nullopt;
    result[slot] = frequency;
  }
  return result;
}
} // namespace drumfoundry::editing
