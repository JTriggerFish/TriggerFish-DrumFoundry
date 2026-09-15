#include "workbench.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
namespace drumfoundry::ui {
void Workbench::SetupFiles() {
  addChild(&presetShade_, false);
  presetShade_.setOnTop(true);
  presetShade_.addChild(&presetSave_);
  presetShade_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(colours::Overlay);
    c.fill(0, 0, width(), height());
  };
  presetSave_.onVisibilityChange() = [this] {
    if (!presetSave_.isVisible())
      presetShade_.setVisible(false);
  };
  presetSave_.error =
      files_.error = [this](const auto &text) { Error(text); };
  addChild(&fileShade_, false);
  fileShade_.setOnTop(true);
  fileShade_.addChild(&files_);
  fileShade_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(colours::Overlay);
    c.fill(0, 0, width(), height());
  };
  files_.onVisibilityChange() = [this] {
    if (!files_.isVisible())
      fileShade_.setVisible(false);
  };
}
} // namespace drumfoundry::ui
