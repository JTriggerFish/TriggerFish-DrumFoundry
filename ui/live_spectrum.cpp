#include "live_spectrum.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
LiveSpectrum::LiveSpectrum() {
  help = "Live FFT of the complete mono output after master gain and the "
         "limiter, including reference playback. Fixed dBFS/bin scale; no "
         "normalization. The comparison spectrogram uses the unlimited render "
         "instead. Hann window, 8192 samples.";
}
void LiveSpectrum::Poll(const Bridge &bridge) {
  if (!bridge.readOutput)
    return;
  const auto read =
      bridge.readOutput(samples_.data(), unsigned(samples_.size()));
  if (read.generation != last_.generation || read.dropped != last_.dropped ||
      read.rate != last_.rate) {
    spectrum_.Reset(read.rate);
    running_ = false;
    redraw();
  }
  last_ = read;
  if (read.samples) {
    spectrum_.Update(samples_.data(), read.samples);
    received_ = std::chrono::steady_clock::now();
    running_ = true;
    redraw();
  } else if (running_ &&
             (!read.rate || std::chrono::steady_clock::now() - received_ >
                                std::chrono::milliseconds(400))) {
    spectrum_.Reset(read.rate);
    running_ = false;
    redraw();
  }
}
void LiveSpectrum::draw(visage::Canvas &c) {
  Label(c,
        running_ ? "LIVE OUTPUT  /  dBFS per bin" : "LIVE OUTPUT  /  stopped",
        0, 0, width(), 20, 0xff8799ae);
  const float left = 30, top = 24, w = std::max(1.f, width() - 34),
              h = std::max(1.f, height() - 48);
  const auto x = [&](double hz) {
    return left + w * float(std::log(hz / 20) / std::log(1000.));
  };
  const auto y = [&](double db) {
    return top + h * float(std::clamp(-db / 96, 0., 1.));
  };
  for (double db : {0., -48., -96.}) {
    c.setColor(0xff293440);
    c.fill(left, y(db), w, 1);
    Label(c, std::to_string(int(db)), 0, y(db) - 8, 28, 16);
  }
  for (double hz : {100., 1000., 10000.}) {
    c.setColor(0xff293440);
    c.fill(x(hz), top, 1, h);
    Label(c,
          hz == 100    ? "100"
          : hz == 1000 ? "1k"
                       : "10k",
          x(hz) - 10, top + h + 2, 32, 18);
  }
  DrawTrace(c, left, top, w, h, 0xff9fcaff);
}
void LiveSpectrum::DrawTrace(visage::Canvas &c, float left, float top, float w,
                             float h, unsigned colour, bool filled) const {
  if (!running_ || !spectrum_.Rate() || w < 1 || h < 1)
    return;
  const auto y = [&](double db) {
    return top + h * float(std::clamp(-db / 96, 0., 1.));
  };
  visage::Path curve;
  if (filled)
    curve.moveTo(left, top + h);
  const auto &bins = spectrum_.Decibels();
  const double binHz = double(spectrum_.Rate()) / analysis::LiveSpectrum::Size;
  // Peak-pool into log-frequency columns so narrow ridges are never skipped.
  for (int column = 0; column <= int(w); ++column) {
    const double a = 20 * std::pow(1000., column / double(w));
    const double b = 20 * std::pow(1000., (column + 1) / double(w));
    const unsigned first = unsigned(std::max(0., std::floor(a / binHz)));
    const unsigned end =
        std::min(unsigned(bins.size()), unsigned(std::ceil(b / binHz)) + 1);
    float db = -120;
    for (unsigned i = first; i < end; ++i)
      db = std::max(db, bins[i]);
    if (!column && !filled)
      curve.moveTo(left, y(db));
    else
      curve.lineTo(left + column, y(db));
  }
  c.setColor(colour);
  if (filled) {
    curve.lineTo(left + w, top + h);
    curve.close();
    c.fill(curve);
  } else
    c.fill(curve.stroke(1.5f));
}
} // namespace drumfoundry::ui
