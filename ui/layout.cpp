#include "workbench.hpp"
#include <algorithm>
#include <cstdio>

namespace drumfoundry::ui {
void Workbench::resized() {
  const float left = std::min(640.f, width() * .46f);
  preset_.setBounds(16, 48, 150, 30);
  stop_.setBounds(174, 48, 80, 30);
  limiter_.setBounds(width() - 270, 48, 125, 30);
  settings_.setBounds(width() - 130, 10, 114, 28);
  master_.setBounds(width() - 510, 42, 220, 44);
  const float col = (left - 48) / 2;
  hardness_.setBounds(16, 154, col, 44);
  implement_.setBounds(16, 210, col, 44);
  location_.setBounds(32 + col, 154, col, 44);
  mute_.setBounds(32 + col, 210, col, 44);
  strike_.setBounds(left + 16, height() * .5f, width() - left - 32, 110);
}
void Workbench::draw(visage::Canvas &c) {
  const float left = std::min(640.f, width() * .46f);
  c.setColor(0xff10151c);
  c.fill(0, 0, width(), height());
  Label(c, "TRIGGERFISH  /  DrumFoundry", 16, 6, 520, 30, 0xffe8b755);
  c.setColor(0xff293440);
  c.fill(0, 98, width(), 1);
  c.fill(left, 98, 1, height() - 140);
  Label(c, "EXCITATION & OUTPUT", 16, 110, left / 2 - 20, 24);
  Label(c, "RESONANCE & BLOOM", left / 2 + 8, 110, left / 2 - 20, 24);
  Label(c, "WAVEFORM / SPECTROGRAM", left + 16, 110, width() - left - 32, 24);
  Label(c, "Native analysis port in progress — no placeholder measurements",
        left + 16, 150, width() - left - 32, 24, 0xff8799ae);
  Label(c, "MODAL PACKET DESIGN", left + 16, height() * .5f + 128,
        width() - left - 32, 24);
  Label(c, "Modal and T60 editors are the next migration chunk", left + 16,
        height() * .5f + 158, width() - left - 32, 24, 0xff8799ae);
  char meter[96];
  std::snprintf(meter, sizeof(meter), "Reduction %.1f dB  |  Lookahead %.2f ms",
                reduction_, latency_);
  Label(c, meter, width() - 510, 78, 495, 18,
        reduction_ > .1 ? 0xffffc65c : 0xff8799ae);
  Label(c, status_, 16, height() - 55, width() - 32, 24, 0xff8799ae);
  Label(c, error_, 16, height() - 29, width() - 32, 24, 0xffefb178);
}
} // namespace drumfoundry::ui
