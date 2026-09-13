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

private:
  void AddParameter(editing::Document &, const editing::Parameter &);
  struct Row {
    std::unique_ptr<visage::Frame> frame;
    int height;
  };
  std::vector<Row> rows_;
  std::shared_ptr<int> generation_;
};
} // namespace drumfoundry::ui
