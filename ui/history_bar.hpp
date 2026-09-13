#pragma once
#include "controls.hpp"
#include "editing/files.hpp"
namespace drumfoundry::ui {
class HistoryBar : public visage::Frame {
public:
  HistoryBar();
  void resized() override;
  std::function<editing::Json()> capture;
  std::function<void(const editing::Json &)> restore;
  std::function<void(bool save, const editing::Json &)> file;
  std::function<void(const std::string &)> error;

private:
  editing::Json NamedDocument();
  void Snapshot();
  void SelectSnapshot();
  visage::TextEditor name_;
  visage::UiButton snapshot_{"Snapshot"}, history_{"Saved snapshots"},
      save_{"Save fit"}, load_{"Load fit"};
};
} // namespace drumfoundry::ui
