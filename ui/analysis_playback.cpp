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
    if (!play || !result_ || result_->auditionRate != request_.auditionRate)
      throw std::runtime_error("Wait for the native audition render to finish");
    const auto &pcm =
        reference ? result_->referencePlayback : result_->modelPlayback;
    if (pcm.empty())
      throw std::runtime_error("Choose a reference sample first");
    // Own only PCM, not the potentially large STFT matrices, in the audio
    // slots.
    play(std::make_shared<const std::vector<float>>(pcm), result_->auditionRate,
         reference ? std::pow(10., referenceGain_.Value() / 20) : 1.);
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
} // namespace drumfoundry::ui
