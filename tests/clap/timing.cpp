#include "host.hpp"
#include <algorithm>
#include <cmath>

namespace clap_test {
namespace {
std::vector<float> Sequence(Host &h, uint32_t blockSize) {
  h.plugin->reset(h.plugin);
  std::vector<float> output;
  constexpr uint32_t length = 1024;
  const std::array<uint32_t, 3> times{17, 257, 711};
  for (uint32_t cursor = 0; cursor < length; cursor += blockSize) {
    const auto frames = std::min(blockSize, length - cursor);
    std::array<clap_event_midi_t, 3> notes{};
    std::vector<const clap_event_header_t *> events;
    for (std::size_t i = 0; i < times.size(); ++i) {
      if (times[i] < cursor || times[i] >= cursor + frames)
        continue;
      notes[i] = Note(times[i] - cursor);
      events.push_back(&notes[i].header);
    }
    Require(h.Render(frames, events) == CLAP_PROCESS_CONTINUE,
            "sequence block");
    output.insert(output.end(), h.left.begin(), h.left.begin() + frames);
  }
  return output;
}
} // namespace
void TestTiming(Host &h) {
  h.Set(100, 0);
  for (double rate :
       {8000., 22050., 44100., 48000., 96000., 192000., 384000.}) {
    h.Start(rate);
    Require(h.latency->get(h.plugin) == std::lround(rate * .001),
            "sample-rate-aware lookahead");
    Require(Sequence(h, 32) == Sequence(h, 512),
            "repeated strikes independent of host buffer size");
    h.Stop();
    Require(h.Get(108) == 0 && h.Get(107) == 0, "inactive output meters clear");
  }
  Require(!h.plugin->activate(h.plugin, 768000, 1, 512),
          "unsupported rate rejected explicitly");
  h.Start();
  clap_event_param_value_t wrong{};
  wrong.header = {sizeof(wrong), 0, 0xb33f, CLAP_EVENT_PARAM_VALUE, 0};
  wrong.param_id = 101;
  wrong.note_id = wrong.port_index = wrong.channel = wrong.key = -1;
  wrong.value = .99;
  const auto before = h.Get(101);
  h.Render(512, {&wrong.header});
  Require(h.Get(101) == before, "foreign namespace cannot change controls");
  h.Stop();
}
} // namespace clap_test
