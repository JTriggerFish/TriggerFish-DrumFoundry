#include "host.hpp"
#include <iostream>

namespace clap_test {
void TestState(Host &h) {
  Require(h.params->count(h.plugin) == 10,
          "small explicit host control surface");
  for (uint32_t i = 0; i < 10; ++i) {
    clap_param_info_t info{};
    Require(h.params->get_info(h.plugin, i, &info), "parameter metadata");
    char text[256]{};
    Require(h.params->value_to_text(h.plugin, info.id, info.default_value, text,
                                    sizeof(text)),
            "display value");
    if (!(info.flags & CLAP_PARAM_IS_READONLY)) {
      double value = -1;
      Require(h.params->text_to_value(h.plugin, info.id, text, &value) &&
                  value == info.default_value,
              "text roundtrip");
    }
  }
  h.Set(100, 5);
  h.Start();
  h.Stop();
  Require(h.Get(102) == .5, "gong preserves saved mallet gesture");
  h.Set(101, .25);
  h.Set(105, -18);
  h.Set(106, 1);
  const auto saved = h.Save();
  h.Set(100, 0);
  h.Set(101, .8);
  h.Set(105, -3);
  h.Set(106, 0);
  h.Start();
  const auto before = h.restarts;
  Require(h.Load(saved), "active project restore validates off audio thread");
  Require(h.restarts == before + 1 && h.latency->get(h.plugin) == 0,
          "state waits for host restart");
  Require(h.Get(100) == 5 && h.Get(101) == .25 && h.Get(105) == -18,
          "restored controls");
  Require(h.Save() == saved, "project preserves complete patch and controls");
  Require(!h.Load("{}") && h.Save() == saved, "invalid state is transactional");
  h.Stop();
  h.Start();
  Require(h.latency->get(h.plugin) == 48,
          "restored protection activates safely");
  const auto note = Note();
  h.Render(512, {&note.header});
  const auto first = h.left;
  h.plugin->reset(h.plugin);
  h.Render(512, {&note.header});
  Require(h.left == first, "reset reproduces first hit");
  h.Stop();
}
} // namespace clap_test
int main(int argc, char **argv) {
  try {
    clap_test::Require(argc == 2, "supply plugin path");
    clap_test::Host first(argv[1]);
    clap_test::Host second(argv[1]);
    clap_test::TestAudio(first);
    clap_test::TestState(first);
    clap_test::TestTiming(first);
    clap_test::Require(second.Get(100) == 0 && second.Get(106) == 1,
                       "instances remain independent");
    std::cout
        << "CLAP loading, MIDI, presets, safety, state and lifecycle passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
