#include "spectrum.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::analysis {
float Spectrogram::Peak(double t0, double t1, double f0, double f1) const {
  if (!frames || !bins || !sampleRate || !hop || !size || !std::isfinite(t0) ||
      !std::isfinite(t1) || !std::isfinite(f0) || !std::isfinite(f1) ||
      t0 > t1 || f0 > f1 || t1 < 0 || f1 < 0 || f0 > sampleRate * .5 ||
      t0 >= double(frames) * hop / sampleRate)
    return -180;
  const auto index = [](double value, double scale, unsigned count) {
    return unsigned(
        std::clamp(std::floor(value * scale + .5), 0., double(count - 1)));
  };
  const auto start = index(t0, double(sampleRate) / hop, frames),
             end = index(t1, double(sampleRate) / hop, frames),
             low = index(f0, double(size) / sampleRate, bins),
             high = index(f1, double(size) / sampleRate, bins);
  float peak = -180;
  for (unsigned frame = start; frame <= end; ++frame) {
    const auto row = db.data() + std::size_t(frame) * bins;
    for (unsigned bin = low; bin <= high; ++bin)
      peak = std::max(peak, row[bin]);
  }
  return peak;
}
} // namespace drumfoundry::analysis
