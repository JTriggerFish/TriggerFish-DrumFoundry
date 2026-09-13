#pragma once
#include "bridge.hpp"
#include "controls.hpp"
#include "workbench/analysis/live_spectrum.hpp"
#include <chrono>
namespace drumfoundry::ui {
// Actual post-master/protection output, separate from the unlimited fit render.
class LiveSpectrum : public visage::Frame, public HelpText {
public:
  LiveSpectrum();
  void Poll(const Bridge &);
  void draw(visage::Canvas &) override;
  unsigned Rate() const { return spectrum_.Rate(); }
  void DrawTrace(visage::Canvas &, float left, float top, float width,
                 float height, unsigned colour, bool filled = false) const;

private:
  analysis::LiveSpectrum spectrum_;
  std::array<float, host::AudioTap::Capacity> samples_{};
  host::TapRead last_;
  std::chrono::steady_clock::time_point received_{};
  bool running_{};
};
} // namespace drumfoundry::ui
