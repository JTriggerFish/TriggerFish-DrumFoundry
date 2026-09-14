#include "analysis_panel.hpp"
#include "analysis_validation.hpp"
#include "editing/files.hpp"
#include "toolbar_layout.hpp"
#include <cmath>
namespace drumfoundry::ui {
AnalysisPanel::AnalysisPanel(std::filesystem::path librarySettings)
    : librarySettings_(std::move(librarySettings)) {
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &view_, &referenceButton_, &fft_, &window_, &comparison_, &reset_,
           &channel_, &duration_, &range_, &referenceGain_,
           &referenceVisible_, &overlap_, &render_, &previousReference_,
           &nextReference_, &referencePlay_})
    addChild(frame);
  referenceButton_.onToggle() = [this](auto *, bool) { ReferenceMenu(); };
  referencePlay_.setVisible(false);
  referencePlay_.onToggle() = [this](auto *, bool) {
    if (requestReferencePlay)
      requestReferencePlay();
  };
  previousReference_.onToggle() = [this](auto *, bool) { StepReference(-1); };
  nextReference_.onToggle() = [this](auto *, bool) { StepReference(1); };
  referenceVisible_.onToggle() = [this](auto *, bool) {
    SetReferenceVisible(!reference_.value("visible", true));
  };
  overlap_.onToggle() = [this](auto *, bool) { OverlapMenu(); };
  render_.onToggle() = [this](auto *, bool) { RenderMenu(); };
  reset_.onToggle() = [this](auto *, bool) { view_.ResetZoom(); };
  duration_.committed = [this] {
    matchLength_ = false;
    view_.span = duration_.Value();
    Queue();
  };
  range_.changed = [this](double v) {
    view_.rangeDb = v;
    view_.Refresh();
  };
  referenceGain_.changed = [this](double v) {
    view_.referenceGainDb = v;
    if (!reference_.is_null())
      reference_["referenceGainDb"] = v;
    view_.Refresh();
  };
  Menus();
}
void AnalysisPanel::SetDocument(const editing::Json &document) {
  ValidateAnalysisDocument(document);
  matchLength_ = false;
  const bool first = request_.document.is_null();
  const bool different =
      first || request_.document.value("id", "") != document.value("id", "");
  request_.document = document;
  published_ = {{"reference", document.value("reference", editing::Json())},
                {"analysis", document.at("controls").at("analysis")}};
  reference_ = document.value("reference", editing::Json());
  ResolveReference();
  const auto a = document.at("controls").at("analysis");
  request_.transform = {a.at("size").get<unsigned>(),
                        a.at("hop").get<unsigned>(), a.at("window")};
  fft_.setText("FFT " + std::to_string(request_.transform.size));
  window_.setText(request_.transform.window);
  range_.Set(a.value("dynamicRangeDb", 80.));
  view_.rangeDb = range_.Value();
  referenceGain_.Set(
      reference_.is_object() ? reference_.value("referenceGainDb", 0.) : 0.);
  view_.referenceGainDb = referenceGain_.Value();
  if (different) {
    duration_.Set(reference_.is_object() ? reference_.value("duration", 8.)
                                         : 8.);
    view_.span = duration_.Value();
    view_.pan = 0;
    view_.modelOffset = 0;
  }
  LoadView(a);
  ApplyReferenceView();
  RefreshTransformLabels();
  Queue();
}
editing::Json AnalysisPanel::Settings() const {
  return {{"size", request_.transform.size},
          {"hop", request_.transform.hop},
          {"window", request_.transform.window},
          {"floorDb", -180},
          {"dynamicRangeDb", range_.Value()},
          {"view",
           {{"comparison", int(view_.comparison)},
            {"span", view_.span},
            {"pan", view_.pan},
            {"split", view_.split},
            {"modelOffset", view_.modelOffset},
            {"frequencyLow", view_.frequencyLow},
            {"frequencyHigh", view_.frequencyHigh},
            {"differenceDb", view_.differenceDb},
            {"renderSeconds", duration_.Value()},
            {"analysisShare", analysisShare},
            {"leftShare", leftShare},
            {"showSpectrogram", showSpectrogram ? 1 : 0},
            {"showModalEditor", showModalEditor ? 1 : 0},
            {"singleColumn", singleColumn ? 1 : 0},
            {"textSize", textSize}}}};
}
void AnalysisPanel::Queue() {
  if (request_.document.is_null())
    return;
  liveMode_ = false;
  liveWorker_.Cancel();
  progressive_.reset();
  referenceContext_.reset();
  request_.duration = matchLength_ ? 0 : duration_.Value();
  view_.renderDuration = duration_.Value();
  worker_.Submit(request_);
  RefreshTransformLabels();
  status_ = "Rendering — previous plot retained";
  redraw();
}
float AnalysisPanel::LayoutControls(float toolsTop) {
  ToolbarLayout reference(width(), 0, 46);
  reference.Place(referenceButton_, 184);
  if (reference_.is_object()) {
    reference.Place(referencePlay_, 32);
    reference.Place(previousReference_, 80);
    reference.Place(nextReference_, 64);
    reference.Place(referenceVisible_, 125);
    reference.Place(referenceGain_, 146, 44);
    reference.Place(channel_, 100);
  }
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &view_, &fft_, &window_, &overlap_, &render_, &reset_, &range_,
           &duration_})
    frame->setVisible(showSpectrogram);
  comparison_.setVisible(showSpectrogram && view_.showReference);
  if (!showSpectrogram)
    return reference.Bottom() + 26;
  auto tools = [this](float y) {
    ToolbarLayout row(width(), y, 34);
    if (comparison_.isVisible())
      row.Place(comparison_, 110);
    row.Place(window_, 158);
    row.Place(fft_, 90);
    row.Place(overlap_, 110);
    row.Place(render_, 135);
    row.Place(reset_, 90);
    ToolbarLayout scales(width(), row.Bottom(), 44);
    scales.Place(range_, 210, 44);
    scales.Place(duration_, 180, 44);
    return scales.Bottom();
  };
  const float controlsHeight = tools(0);
  // Retain the view height: reclaimed space now belongs to the heatmap.
  constexpr float MinimumView = 300;
  const float minimum =
      reference.Bottom() + MinimumView + 26 + controlsHeight;
  const float bottom = std::max(reference.Bottom() + MinimumView,
                                toolsTop - 26 - controlsHeight);
  view_.setBounds(0, reference.Bottom(), width(),
                  bottom - reference.Bottom());
  tools(bottom);
  return minimum;
}
float AnalysisPanel::MinimumHeight() { return LayoutControls(height()); }
void AnalysisPanel::resized() { LayoutControls(height()); }
void AnalysisPanel::draw(visage::Canvas &c) {
  Label(c, referenceWarning_.empty() ? status_ : referenceWarning_, 0,
        height() - 26, width(), 24, colours::Muted);
}
} // namespace drumfoundry::ui
