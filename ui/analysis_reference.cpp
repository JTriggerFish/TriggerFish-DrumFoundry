#include "analysis_panel.hpp"

namespace drumfoundry::ui {
void AnalysisPanel::ResolveReference() {
  request_.reference.clear();
  request_.expectedHash.clear();
  referenceWarning_.clear();
  referenceReady_ = false;
  try {
    libraryRoot_ = analysis::ReadLibraryRoot(librarySettings_);
    reference_ = analysis::PortableReference(reference_, libraryRoot_);
    if (reference_.is_object()) {
      request_.reference = analysis::ResolveLibrarySample(
          libraryRoot_, reference_.value("libraryPath", ""));
      request_.expectedHash = reference_.value("sha256", "");
    }
  } catch (const std::exception &e) {
    referenceWarning_ = std::string("Reference unavailable: ") + e.what();
  }
  request_.channel = analysis::Channel(
      reference_.is_object() ? reference_.value("channel", 0) : 0);
  channel_.setText(request_.channel == analysis::Channel::Left ? "Left"
                   : request_.channel == analysis::Channel::Right
                       ? "Right"
                       : "Mono average");
  referenceGain_.Set(
      reference_.is_object() ? reference_.value("referenceGainDb", 0.) : 0.);
  view_.referenceGainDb = referenceGain_.Value();
  view_.referenceOffset =
      reference_.is_object()
          ? reference_.value("offsetSeconds",
                             reference_.value("cell", editing::Json::object())
                                 .value("onset_seconds", 0.))
          : 0.;
  ApplyReferenceView();
}
void AnalysisPanel::ApplyReferenceView() {
  const bool selected = reference_.is_object();
  referenceButton_.setText(selected
                               ? "Ref: " + reference_.value("name", "Sample")
                               : "Reference: None");
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &referenceVisible_, &referenceGain_, &channel_,
           &previousReference_, &nextReference_, &referencePlay_})
    frame->setVisible(selected);
  const bool visible = selected && reference_.value("visible", true);
  referenceVisible_.setText(visible ? "Hide reference" : "Show reference");
  view_.showReference = visible && referenceReady_;
  comparison_.setVisible(showSpectrogram && view_.showReference);
  view_.Refresh();
  resized();
  if (layoutChanged)
    layoutChanged();
}
void AnalysisPanel::ClearReference() {
  reference_ = nullptr;
  matchLength_ = false;
  ResolveReference();
  Queue();
}
void AnalysisPanel::SetReferenceVisible(bool visible) {
  if (!reference_.is_object())
    return;
  reference_["visible"] = visible;
  ApplyReferenceView();
  PublishState();
}
void AnalysisPanel::SetLibraryRoot(const std::filesystem::path &root) {
  analysis::SaveLibraryRoot(root, librarySettings_);
  referenceFolder_.clear();
  ResolveReference();
  Queue();
}
void AnalysisPanel::SetReference(const std::filesystem::path &path) {
  const auto relative = analysis::LibraryRelativePath(libraryRoot_, path);
  reference_ = {{"id", ""},
                {"sha256", ""},
                {"name", path.filename().u8string()},
                {"libraryPath", relative},
                {"visible", true},
                {"offsetSeconds", 0.},
                {"referenceGainDb", referenceGain_.Value()},
                {"channel", 0}};
  ResolveReference();
  matchLength_ = true;
  Queue();
}
editing::Json AnalysisPanel::Reference() const {
  auto ref = reference_;
  if (ref.is_object()) {
    ref.erase("localPath");
    ref["referenceGainDb"] = referenceGain_.Value();
    ref["channel"] = int(request_.channel);
    ref["offsetSeconds"] = view_.referenceOffset;
  }
  return ref;
}
} // namespace drumfoundry::ui
