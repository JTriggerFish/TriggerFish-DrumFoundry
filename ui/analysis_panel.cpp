#include "analysis_panel.hpp"
#include "analysis_validation.hpp"
#include "editing/files.hpp"
#include "toolbar_layout.hpp"
#include <cmath>
namespace drumfoundry::ui {
AnalysisPanel::AnalysisPanel() {
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &view_, &referenceButton_, &fft_, &window_, &comparison_, &reset_,
           &channel_, &duration_, &range_, &referenceGain_, &articulation_,
           &layer_, &take_, &overlap_, &render_})
    addChild(frame);
  referenceButton_.onToggle() = [this](auto *, bool) { ReferenceMenu(); };
  articulation_.onToggle() = [this](auto *, bool) {
    ReferenceDimensionMenu("articulation", articulation_);
  };
  layer_.onToggle() = [this](auto *, bool) {
    ReferenceDimensionMenu("velocity", layer_);
  };
  take_.onToggle() = [this](auto *, bool) {
    ReferenceDimensionMenu("repeat", take_);
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
  published_ = {{"reference", document.at("reference")},
                {"analysis", document.at("controls").at("analysis")}};
  reference_ = document.value("reference", editing::Json());
  request_.reference =
      reference_.is_object()
          ? std::filesystem::u8path(reference_.value("localPath", ""))
          : std::filesystem::path();
  const auto catalogPath =
      editing::FitDirectory().parent_path() / "references" / "catalog.json";
  if (catalog_.Cells().empty() && std::filesystem::exists(catalogPath))
    catalog_.Load(catalogPath);
  if (request_.reference.empty() ||
      !std::filesystem::exists(request_.reference))
    if (const auto *cell = catalog_.Find(reference_)) {
      request_.reference = cell->path;
      reference_["localPath"] = cell->path.u8string();
    }
  referenceButton_.setText(reference_.is_object()
                               ? reference_.value("name", "Reference")
                               : "Reference WAV");
  hashPending_ = false;
  request_.expectedHash =
      reference_.is_object() ? reference_.value("sha256", "") : "";
  const int channel =
      reference_.is_object() ? reference_.value("channel", 0) : 0;
  if (channel < 0 || channel > 2)
    throw std::invalid_argument("Invalid reference channel");
  request_.channel = analysis::Channel(channel);
  channel_.setText(channel == 0   ? "Mono average"
                   : channel == 1 ? "Left"
                                  : "Right");
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
    view_.referenceOffset =
        reference_.is_object() && reference_.contains("cell")
            ? reference_.at("cell").value("onset_seconds", 0.)
            : 0;
  }
  LoadView(a);
  RefreshReferenceControls();
  RefreshTransformLabels();
  Queue();
}
void AnalysisPanel::SetReference(const std::filesystem::path &path) {
  request_.reference = std::filesystem::absolute(path);
  request_.expectedHash.clear();
  referenceButton_.setText(path.filename().u8string());
  hashPending_ = true;
  matchLength_ = true;
  reference_ = {{"id", ""},
                {"sha256", ""},
                {"name", path.filename().u8string()},
                {"localPath", request_.reference.u8string()},
                {"referenceGainDb", referenceGain_.Value()}};
  view_.referenceOffset = 0;
  RefreshReferenceControls();
  Queue();
}
editing::Json AnalysisPanel::Reference() const {
  if (hashPending_)
    throw std::runtime_error(
        "Reference is still loading; wait before saving a snapshot");
  auto ref = reference_;
  if (ref.is_object()) {
    ref["referenceGainDb"] = referenceGain_.Value();
    ref["channel"] = int(request_.channel);
    if (!ref.contains("cell"))
      ref["cell"] = editing::Json::object();
    ref["cell"]["onset_seconds"] = view_.referenceOffset;
  }
  return ref;
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
            {"analysisShare", analysisShare}}}};
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
void AnalysisPanel::resized() {
  ToolbarLayout reference(width(), 0, 46);
  reference.Place(referenceButton_, 184);
  if (catalog_.Find(reference_)) {
    reference.Place(articulation_, 104);
    reference.Place(layer_, 94);
    reference.Place(take_, 64);
  }
  reference.Place(referenceGain_, 146, 44);
  reference.Place(channel_, 100);
  // Measure wrapped tool rows first; the waveform and plot precede them.
  auto tools = [this](float y) {
    ToolbarLayout row(width(), y, 34);
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
  const float bottom =
      std::max(reference.Bottom() + 180, height() - 26 - controlsHeight);
  view_.setBounds(0, reference.Bottom(), width(), bottom - reference.Bottom());
  tools(bottom);
}
void AnalysisPanel::draw(visage::Canvas &c) {
  Label(c, status_, 0, height() - 26, width(), 24, 0xff8799ae);
}
} // namespace drumfoundry::ui
