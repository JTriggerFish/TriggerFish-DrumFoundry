#pragma once
#include "controls.hpp"
#include "editing/document.hpp"
#include <visage_ui/scroll_bar.h>

namespace drumfoundry::ui {
class LiveSpectrum;
// Scroll independently from analysis/strike controls. All scalar controls
// come from engine metadata; specialized curve editors replace their scalar
// rows.
class ParameterPanel : public visage::ScrollableFrame {
public:
  void Load(editing::Document &, bool rightColumn);
  void resized() override;
  void draw(visage::Canvas &) override;
  std::function<void()> committed;
  std::function<void(const std::string &)> error;
  std::function<void(bool size)> meta;
  visage::Frame
      *holdDecay{}; // Borrowed design-time tool in the bloom section.
  LiveSpectrum
      *outputSpectrum{}; // Borrowed from the workbench, never DSP-owned.
  std::function<unsigned()> previewRate;
  void RefreshSpectrum() {
    if (preview_)
      preview_->redraw();
  }

private:
  void AddGroup(const std::string &section);
  void AddParameter(editing::Document &, const editing::Parameter &);
  void AddOutputPreview(editing::Document &);
  void SyncValues(editing::Document &);
  struct Row {
    Row(std::unique_ptr<visage::Frame> item, int h)
        : owner(std::move(item)), frame(owner.get()), height(h) {}
    Row(visage::Frame &item, int h) : frame(&item), height(h) {}
    std::unique_ptr<visage::Frame> owner;
    visage::Frame *frame;
    int height;
  };
  std::vector<Row> rows_;
  // Content-space bounds: cards scroll with their rows without intercepting
  // input.
  struct Group {
    std::size_t firstRow;
    float top{}, height{};
  };
  std::vector<Group> groups_;
  std::vector<std::pair<Slider *, std::string>> sliders_;
  visage::Frame *preview_{};
  std::shared_ptr<int> generation_;
};
} // namespace drumfoundry::ui
