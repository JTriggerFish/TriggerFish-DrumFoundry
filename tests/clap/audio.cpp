#include "host.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace clap_test {
void TestAudio(Host &h) {
  const auto *audio = static_cast<const clap_plugin_audio_ports_t *>(
      h.plugin->get_extension(h.plugin, CLAP_EXT_AUDIO_PORTS));
  const auto *notes = static_cast<const clap_plugin_note_ports_t *>(
      h.plugin->get_extension(h.plugin, CLAP_EXT_NOTE_PORTS));
  Require(audio && audio->count(h.plugin, false) == 1 &&
              !audio->count(h.plugin, true),
          "stereo output, no input");
  clap_note_port_info_t noteInfo{};
  Require(notes && notes->get(h.plugin, 0, true, &noteInfo) &&
              noteInfo.supported_dialects == CLAP_NOTE_DIALECT_MIDI,
          "honest MIDI dialect");
  for (int preset = 0; preset < 6; ++preset) {
    h.Set(100, preset);
    h.Start();
    Require(h.latency->get(h.plugin) == 48 && h.Get(108) == 1,
            "one millisecond lookahead");
    const auto note = Note(17);
    Require(h.Render(512, {&note.header}) == CLAP_PROCESS_CONTINUE,
            "MIDI strike renders");
    for (int i = 0; i < 65; ++i)
      Require(h.left[i] == 0, "sample-accurate strike plus limiter delay");
    double energy = 0;
    for (int block = 0; block < 48; ++block) {
      for (std::size_t i = 0; i < h.left.size(); ++i) {
        Require(std::isfinite(h.left[i]) && std::abs(h.left[i]) < .9f,
                "protected finite audio");
        Require(h.left[i] == h.right[i],
                "mono voice presented consistently in stereo");
        energy += h.left[i] * h.left[i];
      }
      Require(h.Render(512) == CLAP_PROCESS_CONTINUE, "tail rendering");
    }
    Require(energy > 1e-6, "each embedded instrument produces audio");
    h.Stop();
  }
  h.Set(100, 0);
  h.Start();
  const auto previous = h.restarts;
  h.Set(106, 0);
  Require(h.restarts == previous + 1 && h.latency->get(h.plugin) == 48,
          "bypass requests a safe restart");
  h.Stop();
  h.Start();
  Require(h.latency->get(h.plugin) == 0 && h.Get(108) == 0,
          "bypass has zero latency");
  auto bad = Note(512);
  Require(h.Render(512, {&bad.header}) == CLAP_PROCESS_ERROR,
          "reject event outside block");
  Require(
      std::all_of(h.left.begin(), h.left.end(), [](float v) { return v == 0; }),
      "silence invalid event block");
  bad = Note();
  bad.header.size = sizeof(clap_event_header_t);
  Require(h.Render(512, {&bad.header}) == CLAP_PROCESS_ERROR,
          "reject undersized MIDI");
  const auto late = Note(20), early = Note(10);
  Require(h.Render(512, {&late.header, &early.header}) == CLAP_PROCESS_ERROR,
          "reject unordered events");
  h.Set(101, std::numeric_limits<double>::quiet_NaN());
  Require(std::isfinite(h.Get(101)), "reject nonfinite controls");
  h.Stop();
}
} // namespace clap_test
