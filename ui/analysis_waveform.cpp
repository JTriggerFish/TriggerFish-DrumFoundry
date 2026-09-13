#include "analysis_view.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace drumfoundry::ui {
// Waveforms always run forward in time; the spectrogram alone is mirrored.
// Both lanes use the reference amplitude scale, never model auto-normalisation.
void AnalysisView::Waveform(visage::Canvas &c) {
  const double gain = std::pow(10., referenceGainDb / 20);
  double peak = .001;
  for (float v : result_->reference.samples)
    peak = std::max(peak, std::abs(v * gain));
  if (result_->reference.samples.empty())
    peak = 1;
  const int pixels = std::max(1, int(width() - 54));
  for (int source = 0; source < 2; ++source) {
    const auto &audio = source ? result_->model : result_->reference;
    const double offset = source ? modelOffset : referenceOffset;
    const float centre = source ? 57.f : 21.f;
    const unsigned colour = source ? 0xff8bbbeb : 0xffd6b25c;
    c.setColor(0xff293440);
    c.fill(42, centre, pixels, 1);
    Label(c, source ? "Synth" : "Ref", 0, centre - 9, 40, 18, colour);
    c.setColor(colour);
    if (!audio.sampleRate)
      continue;
    for (int x = 0; x < pixels; ++x) {
      const auto &segment =
          source ? ModelAt(pan + offset + span * x / pixels).model : audio;
      const auto first =
          int64_t((pan + offset + span * x / pixels) * segment.sampleRate);
      const auto last = int64_t((pan + offset + span * (x + 1) / pixels) *
                                segment.sampleRate);
      float low = 0, high = 0;
      for (auto i = std::max<int64_t>(0, first);
           i <
           std::min<int64_t>(segment.samples.size(), std::max(first + 1, last));
           ++i) {
        const float v =
            segment.samples[std::size_t(i)] * float(source ? 1 : gain);
        low = std::min(low, v);
        high = std::max(high, v);
      }
      const float top = float(std::clamp(high / peak, -1., 1.)) * 15;
      const float bottom = float(std::clamp(low / peak, -1., 1.)) * 15;
      // Silence is represented by the baseline, not a coloured block.
      if (high != 0 || low != 0)
        c.fill(42 + x, centre - top, 1, std::max(.5f, top - bottom));
    }
  }
  for (int i = 0; i <= 4; ++i) {
    const float x = 42 + pixels * i / 4.f;
    char text[48];
    std::snprintf(text, sizeof(text), "%.2f s", pan + span * i / 4);
    Label(c, text, std::clamp(x - 20, 42.f, width() - 65), 77, 65, 18,
          0xff8799ae);
  }
}
} // namespace drumfoundry::ui
