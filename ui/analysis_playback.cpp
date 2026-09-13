#include "analysis_panel.hpp"
#include <cmath>
#include <stdexcept>
namespace drumfoundry::ui {
void AnalysisPanel::SetAuditionRate(unsigned rate) {
  if (!rate || rate == request_.auditionRate)
    return;
  request_.auditionRate = rate;
  Queue();
}
void AnalysisPanel::Play(bool reference) {
  try {
    if (reference && (!reference_.is_object() || !referenceReady_))
      throw std::runtime_error("No reference sample available; select one from "
                               "the reference library");
    if (!reference && (worker_.Busy() || (liveMode_ && !liveComplete_)))
      throw std::runtime_error("Wait for the current capture/render to finish, "
                               "or use Strike to play live");
    const auto source =
        reference && referenceContext_ ? referenceContext_ : result_;
    if (!play || !source || source->auditionRate != request_.auditionRate)
      throw std::runtime_error("Wait for the native audition render to finish");
    const auto &pcm =
        reference ? source->referencePlayback : source->modelPlayback;
    if (pcm.empty())
      throw std::runtime_error("Choose a reference sample first");
    liveWorker_.Cancel();
    liveMode_ = false;
    // Own only PCM, not the potentially large STFT matrices, in the audio
    // slots.
    play(std::make_shared<const std::vector<float>>(pcm), source->auditionRate,
         reference ? std::pow(10., referenceGain_.Value() / 20) : 1.);
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
} // namespace drumfoundry::ui
