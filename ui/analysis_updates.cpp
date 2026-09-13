#include "analysis_panel.hpp"
namespace drumfoundry::ui {
void AnalysisPanel::Poll() {
  if (auto context = worker_.TakeContext())
    ReceiveContext(std::move(context));
  const auto chunks = worker_.TakeChunks();
  if (!liveMode_)
    AppendPreview(chunks);
  if (auto result = worker_.Take())
    ReceiveResult(std::move(result));
  PublishState();
}
void AnalysisPanel::ReceiveContext(
    std::shared_ptr<const analysis::Result> context) {
  AcceptReference(*context);
  referenceContext_ = context;
  if (liveMode_) {
    // First strike can arrive before the reference has decoded. Retain its
    // live audio, attach the new reference, then cancel redundant synthesis.
    progressive_->reference = context->reference;
    progressive_->referenceSpectrum = context->referenceSpectrum;
    progressive_->referenceHash = context->referenceHash;
    progressive_->referencePlayback = context->referencePlayback;
    progressive_->auditionRate = context->auditionRate;
    result_ = context;
    if (liveComplete_)
      result_ = progressive_;
    const auto &s = progressive_->modelSpectrum;
    view_.SetProgress(progressive_,
                      s.sampleRate ? double(s.frames) * s.hop / s.sampleRate
                                   : 0);
    view_.Refresh(); // Reference-anchored colour ceiling has just changed.
    worker_.Cancel();
  } else
    progressive_ = std::make_shared<analysis::Result>(*context);
}
void AnalysisPanel::ReceiveResult(
    std::shared_ptr<const analysis::Result> result) {
  if (!result->error.empty()) {
    status_ = "Render failed — previous plot retained";
    if (error)
      error(result->error);
  } else {
    result_ = std::move(result);
    if (matchLength_) {
      duration_.Set(double(result_->model.samples.size()) /
                    result_->model.sampleRate);
      view_.span = duration_.Value();
      view_.pan = 0;
      matchLength_ = false;
    }
    view_.renderDuration = duration_.Value();
    view_.Set(result_);
    AcceptReference(*result_);
    status_ =
        "Native render + STFT: " + std::to_string(int(result_->elapsedMs)) +
        " ms · " + std::to_string(result_->model.sampleRate) + " Hz";
  }
  redraw();
}
void AnalysisPanel::AcceptReference(const analysis::Result &context) {
  if (request_.reference.empty() || !context.reference.sampleRate)
    return;
  reference_["id"] = "sha256:" + context.referenceHash;
  reference_["sha256"] = context.referenceHash;
  reference_["sampleRate"] = context.reference.sampleRate;
  reference_["channels"] = context.reference.sourceChannels;
  reference_["duration"] =
      double(context.reference.samples.size()) / context.reference.sampleRate;
  hashPending_ = false;
  if (matchLength_) {
    duration_.Set(reference_.at("duration"));
    view_.span = view_.renderDuration = duration_.Value();
    view_.pan = 0;
    matchLength_ = false;
  }
}
void AnalysisPanel::AppendPreview(
    const std::vector<analysis::PreviewChunk> &chunks) {
  if (!progressive_ || chunks.empty())
    return;
  for (const auto &chunk : chunks) {
    if (chunk.firstSample != progressive_->model.samples.size() ||
        chunk.firstFrame != progressive_->modelSpectrum.frames)
      throw std::runtime_error("Preview stream discontinuity");
    auto &samples = progressive_->model.samples;
    samples.insert(samples.end(), chunk.audio.samples.begin(),
                   chunk.audio.samples.end());
    progressive_->model.sampleRate = chunk.audio.sampleRate;
    analysis::AppendSpectrum(progressive_->modelSpectrum, chunk.spectrum);
  }
  const auto &s = progressive_->modelSpectrum;
  if (liveMode_ && chunks.back().complete) {
    liveComplete_ = true;
    progressive_->auditionRate = progressive_->model.sampleRate;
    progressive_->modelPlayback = progressive_->model.samples;
    result_ = progressive_;
    view_.Set(result_);
    status_ =
        "Live capture complete · strike again or edit for a fresh preview";
    redraw();
    return;
  }
  view_.SetProgress(progressive_,
                    s.sampleRate ? double(s.frames) * s.hop / s.sampleRate : 0);
}
} // namespace drumfoundry::ui
