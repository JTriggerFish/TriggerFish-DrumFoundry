#include "ui/analysis_panel.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>
// The panel must display/capture the supplied live stream, never replace
// repeated hits with an offline single-hit synthesis of its saved patch.
void LivePanelTests(drumfoundry::Json document) {
  using namespace drumfoundry;
  document["reference"] = nullptr;
  document["controls"]["analysis"]["view"] = {{"renderSeconds", .25}};
  ui::AnalysisPanel panel;
  panel.error = [](const auto &message) { throw std::runtime_error(message); };
  panel.SetDocument(document);
  std::vector<float> audio(12000);
  for (unsigned i = 0; i < audio.size(); ++i)
    audio[i] = float(.25 * std::sin(i * .1));
  audio[0] = .73f;
  audio[6000] = -.83f;
  unsigned cursor = 0;
  ui::Bridge bridge;
  bridge.readVoice = [&](float *pcm, unsigned maximum) {
    const auto count =
        std::min({maximum, 256u, unsigned(audio.size()) - cursor});
    std::copy_n(audio.data() + cursor, count, pcm);
    host::TapRead read{count, 48000, 1, 0};
    if (cursor == 0 || (cursor <= 6000 && cursor + count > 6000))
      read.firstStrike = read.lastStrike = cursor == 0 ? 0 : 6000 - cursor;
    cursor += count;
    return read;
  };
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  // Strike before initial offline work finishes: reference arrival must not
  // replace the live timeline or leave the panel waiting indefinitely.
  while ((cursor < audio.size() || !panel.Ready()) &&
         std::chrono::steady_clock::now() < deadline) {
    panel.PollLive(bridge);
    panel.Poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  if (!panel.Ready())
    throw std::runtime_error("Live panel did not finish");
  bool played = false;
  panel.play = [&](auto pcm, unsigned rate, double gain) {
    if (*pcm != audio || rate != 48000 || gain != 1)
      throw std::runtime_error("Live stream was replaced or rescaled");
    played = true;
  };
  panel.Play(false);
  if (!played)
    throw std::runtime_error("Live capture playback missing");
}
