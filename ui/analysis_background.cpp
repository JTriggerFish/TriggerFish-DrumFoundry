#include "analysis_view.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
// Flatten an interrupted preview and its old tail once. This bounds history
// memory and preserves the visible composite when zooming during another edit.
std::shared_ptr<const analysis::Result> AnalysisView::FreezeDisplayed() const {
  if (!result_ || !previousResult_ || writtenSeconds_ < 0)
    return result_;
  const auto &fresh = result_->modelSpectrum;
  const auto &old = previousResult_->modelSpectrum;
  if (fresh.sampleRate != old.sampleRate || fresh.size != old.size ||
      fresh.hop != old.hop)
    return result_;
  auto frozen = std::make_shared<analysis::Result>();
  frozen->model = previousResult_->model;
  frozen->modelSpectrum = old;
  auto &audio = frozen->model.samples;
  const auto count = std::min(
      result_->model.samples.size(),
      std::size_t(std::ceil(writtenSeconds_ * result_->model.sampleRate)));
  audio.resize(std::max(audio.size(), count));
  std::copy_n(result_->model.samples.begin(), count, audio.begin());
  auto &spectrum = frozen->modelSpectrum;
  spectrum.frames = std::max(old.frames, fresh.frames);
  spectrum.db.resize(std::size_t(spectrum.frames) * spectrum.bins, -180);
  std::copy(fresh.db.begin(), fresh.db.end(), spectrum.db.begin());
  return frozen;
}
} // namespace drumfoundry::ui
