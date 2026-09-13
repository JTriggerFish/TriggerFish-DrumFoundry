#include "workbench.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
namespace drumfoundry::ui {
void Workbench::SetupFiles() {
  addChild(&history_);
  history_.capture = [this] { return CaptureDocument(); };
  history_.restore = [this](const auto &document) {
    bridge_.applyDocument(document);
    reloadDocument_ = true;
  };
  history_.file = [this](bool save, const auto &document) {
    OpenFitFile(save, document);
  };
  history_.error = files_.error = [this](const auto &text) { Error(text); };
  addChild(&fileShade_, false);
  fileShade_.setOnTop(true);
  fileShade_.addChild(&files_);
  fileShade_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(0xa005090f);
    c.fill(0, 0, width(), height());
  };
  files_.onVisibilityChange() = [this] {
    if (!files_.isVisible())
      fileShade_.setVisible(false);
  };
}
} // namespace drumfoundry::ui
