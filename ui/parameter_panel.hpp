#pragma once
#include "controls.hpp"
#include "editing/document.hpp"
#include <visage_ui/scroll_bar.h>

namespace drumfoundry::ui {
// Scroll independently from analysis/strike controls. All scalar controls come
// from engine metadata; specialized curve editors replace their scalar rows.
class ParameterPanel : public visage::ScrollableFrame {
public:
  void Load(editing::Document &, bool rightColumn);
  void resized() override;
  std::function<void()> committed;
  std::function<void(const std::string &)> error;
  std::function<void(bool size)> meta;
  visage::Frame
      *outputSpectrum{}; // Borrowed from the workbench, never DSP-owned.

private:
  void AddParameter(editing::Document &, const editing::Parameter &);
  struct Row {
    Row(std::unique_ptr<visage::Frame> item, int h)
        : owner(std::move(item)), frame(owner.get()), height(h) {}
    Row(visage::Frame &item, int h) : frame(&item), height(h) {}
    std::unique_ptr<visage::Frame> owner;
    visage::Frame *frame;
    int height;
  };
  std::vector<Row> rows_;
  std::shared_ptr<int> generation_;
};
} // namespace drumfoundry::ui
