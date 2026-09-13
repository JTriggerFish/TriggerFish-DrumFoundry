#include "analysis_panel.hpp"
namespace drumfoundry::ui {
bool AnalysisPanel::PollLive(const Bridge &bridge) {
  if (!bridge.readVoice)
    return false;
  const auto read =
      bridge.readVoice(liveSamples_.data(), unsigned(liveSamples_.size()));
  if (read.generation != lastLive_.generation ||
      read.dropped != lastLive_.dropped) {
    liveWorker_.Cancel();
    liveComplete_ = false;
    if (liveMode_)
      status_ = "Live stream reset — strike to start a new capture";
    if (liveMode_ && read.dropped != lastLive_.dropped && error)
      error("Live display missed audio while the editor was busy. Strike again "
            "to restart capture; audio playback is unaffected.");
  }
  lastLive_ = read;
  const bool struck = read.firstStrike != ~0u;
  unsigned offset = 0;
  if (struck && !liveWorker_.Active()) {
    if (referenceContext_)
      worker_.Cancel();
    liveMode_ = true;
    liveComplete_ = false;
    auto context = referenceContext_
                       ? std::make_shared<analysis::Result>(*referenceContext_)
                       : std::make_shared<analysis::Result>();
    context->model = {read.rate, 1, {}};
    context->modelSpectrum = {};
    context->modelPlayback.clear();
    progressive_ = std::move(context);
    liveWorker_.Begin(read.rate, request_.transform, duration_.Value());
    offset = read.firstStrike;
    status_ = "LIVE instrument · before monitor gain/limiter · repeated hits "
              "accumulate";
  }
  if (liveWorker_.Active() && read.samples > offset)
    liveWorker_.Feed(liveSamples_.data() + offset, read.samples - offset);
  if (liveMode_)
    AppendPreview(liveWorker_.Take());
  const auto failure = liveWorker_.Error();
  if (!failure.empty()) {
    status_ = failure;
    if (error)
      error(failure);
  }
  if (liveMode_)
    redraw();
  return struck;
}
} // namespace drumfoundry::ui
