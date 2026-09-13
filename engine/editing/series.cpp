#include "modes.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace drumfoundry::editing {
namespace {
// Lowest distinct circular-membrane Bessel roots / j_(0,1). Same table and
// protected-core stretching law as the web workbench; not a physical gong law.
constexpr double Membrane[]{1,           1.593340506, 2.135548787, 2.295417267,
                            2.653066405, 2.917295455, 3.155464815, 3.500147490,
                            3.598484674, 3.647451179, 4.058931883, 4.131738160,
                            4.230439128, 4.601044534, 4.610051645, 4.831885263,
                            4.903280573, 5.083567174, 5.130768907, 5.412118430,
                            5.540398510, 5.553126477, 5.650842377, 5.976540222,
                            6.019355807, 6.152609172, 6.163136731, 6.208732131,
                            6.482735446, 6.528612452, 6.668996901, 6.746213300};
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
      s.turbulence < 0 || s.turbulence > 2)
    throw std::invalid_argument("Invalid modal series settings");
  std::vector<Mode> result;
  for (unsigned i = 0; i < s.count; ++i) {
    const double base =
        s.family == SeriesFamily::Membrane ? Membrane[i] : i + 1;
    const double upper =
        std::max(0., (double(i + 1) - s.harmonicCore) / s.harmonicCore);
    const double ratio = base * std::hypot(1., s.stretch * upper);
    if (s.fundamental * ratio > maximum)
      throw std::invalid_argument("Only " + std::to_string(result.size()) +
                                  " modes fit this pitch/stretch range");
    result.push_back(
        {s.fundamental * ratio,
         std::clamp(s.level - s.rolloff * std::log2(ratio), -72., 6.),
         s.turbulence, 1});
  }
  return result;
}
} // namespace drumfoundry::editing
