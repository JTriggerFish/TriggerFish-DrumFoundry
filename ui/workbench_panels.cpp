#include "workbench.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
namespace drumfoundry::ui {
void Workbench::SetupPanels() {
  resonance_.holdDecay = &holdDecay_;
  holdDecay_.error = [this](const auto &text) { Error(text); };
  holdDecay_.apply = [this](const auto &result) {
    document_.SetMany(result.values);
    applyingHold_ = true;
    ApplyDocument();
    applyingHold_ = false;
    reloadDocument_ = true; // Rebuild the visible T60 curve and scalar values.
  };
  excitation_.outputSpectrum = &liveSpectrum_;
  excitation_.previewRate = [this] { return analysis_.RenderRate(); };
  for (auto *panel : {&excitation_, &resonance_}) {
    panel->committed = [this] { ApplyDocument(); };
    panel->error = [this](const auto &text) { Error(text); };
    panel->meta = [this](bool size) {
      meta_.Open(document_, size);
      metaShade_.setVisible(true);
    };
  }
  SetupAnalysis();
  SetupMetas();
}
void Workbench::SetupAnalysis() {
  addChild(&right_);
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &analysis_, &modal_, &strike_, &hardness_, &velocity_, &spread_,
           &analysisSplit_, &fixedStrike_})
    right_.addScrolledChild(frame);
  analysisSplit_.started = [this] { splitStart_ = analysis_.height(); };
  analysisSplit_.dragged = [this](float delta) {
    const double flexible = std::max(1350.f, right_.height()) - 210;
    analysis_.analysisShare =
        std::clamp((splitStart_ + delta) / flexible, .1, .9);
    LayoutRight();
  };
  analysis_.error = [this](const auto &message) { Error(message); };
  analysis_.play = bridge_.play;
  analysis_.presentation = bridge_.presentation;
  referencePlay_.onToggle() = [this](auto *, bool) {
    if (EnsureAudio())
      analysis_.Play(true);
  };
  modelPlay_.onToggle() = [this](auto *, bool) {
    if (EnsureAudio())
      analysis_.Play(false);
  };
  modal_.committed = [this] { ApplyDocument(); };
  modal_.error = [this](const auto &text) { Error(text); };
}
void Workbench::SetupMetas() {
  addChild(&metaShade_, false);
  metaShade_.setOnTop(true);
  metaShade_.addChild(&meta_);
  metaShade_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(0x4005090f);
    c.fill(0, 0, width(), height());
  };
  meta_.onVisibilityChange() = [this] {
    if (!meta_.isVisible())
      metaShade_.setVisible(false);
  };
  meta_.changed = [this] {
    help_.Hide();
    excitation_.Load(document_, false);
    resonance_.Load(document_, true);
    modal_.Refresh();
    ControlErrors(*this, [this](const auto &text) { Error(text); });
    help_.Bind(*this);
  };
  meta_.committed = [this] { ApplyDocument(); };
  meta_.error = [this](const auto &text) { Error(text); };
}
} // namespace drumfoundry::ui
