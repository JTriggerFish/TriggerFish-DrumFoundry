#include "analysis_panel.hpp"
#include "editing/files.hpp"
#include <cmath>
namespace drumfoundry::ui {
AnalysisPanel::AnalysisPanel() {
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &view_, &referenceButton_, &fft_, &window_, &comparison_, &reset_,
           &channel_, &duration_, &range_, &referenceGain_})
    addChild(frame);
  referenceButton_.onToggle() = [this](auto *, bool) { ReferenceMenu(); };
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
  if (request_.reference.empty())
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
            {"renderSeconds", duration_.Value()}}}};
}
void AnalysisPanel::Queue() {
  if (request_.document.is_null())
    return;
  request_.duration = matchLength_ ? 0 : duration_.Value();
  worker_.Submit(request_);
  status_ = "Rendering — previous plot retained";
  redraw();
}
void AnalysisPanel::Poll() {
  if (auto result = worker_.Take()) {
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
      view_.Set(result_);
      if (!request_.reference.empty()) {
        reference_["id"] = "sha256:" + result_->referenceHash;
        reference_["sha256"] = result_->referenceHash;
        reference_["sampleRate"] = result_->reference.sampleRate;
        reference_["channels"] = result_->reference.sourceChannels;
        reference_["duration"] = double(result_->reference.samples.size()) /
                                 result_->reference.sampleRate;
        hashPending_ = false;
      }
      status_ =
          "Native render + STFT: " + std::to_string(int(result_->elapsedMs)) +
          " ms · " + std::to_string(result_->model.sampleRate) + " Hz";
    }
    redraw();
  }
  if (worker_.Busy())
    redraw();
  PublishState();
}
void AnalysisPanel::resized() {
  const float col = (width() - 24) / 3;
  referenceButton_.setBounds(0, 0, col, 28);
  comparison_.setBounds(col + 12, 0, col, 28);
  fft_.setBounds(2 * (col + 12), 0, col, 28);
  window_.setBounds(0, 34, col, 28);
  reset_.setBounds(col + 12, 34, col, 28);
  channel_.setBounds(2 * (col + 12), 34, col, 28);
  duration_.setBounds(0, 68, col, 44);
  range_.setBounds(col + 12, 68, col, 44);
  referenceGain_.setBounds(2 * (col + 12), 68, col, 44);
  view_.setBounds(0, 120, width(), height() - 147);
}
void AnalysisPanel::draw(visage::Canvas &c) {
  Label(c, status_, 0, height() - 26, width(), 24, 0xff8799ae);
  if (worker_.Busy()) {
    c.setColor(0xff8799ae);
    c.fill(0, height() - 3, width() * worker_.Progress(), 2);
  }
}
} // namespace drumfoundry::ui
