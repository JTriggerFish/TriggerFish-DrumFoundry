#include "parameter_panel.hpp"
#include <algorithm>
#include <exception>

namespace drumfoundry::ui {
namespace {
class Heading : public visage::Frame {
public:
  explicit Heading(std::string text) : text_(std::move(text)) {}
  void draw(visage::Canvas &c) override {
    c.setColor(0xff293440);
    c.fill(0, 4, width(), 1);
    Label(c, text_, 0, 8, width(), 24, 0xffe8b755);
  }

private:
  std::string text_;
};
} // namespace
void ParameterPanel::Load(editing::Document &document, bool right) {
  for (auto &row : rows_)
    removeScrolledChild(row.frame.get());
  rows_.clear();
  std::vector<std::string> sections;
  for (const auto &p : document.Parameters()) {
    if (editing::RightColumn(p) != right)
      continue;
    const auto section = editing::Section(p);
    if (std::find(sections.begin(), sections.end(), section) == sections.end())
      sections.push_back(section);
  }
  for (const auto &section : sections) {
    auto heading = std::make_unique<Heading>(section);
    addScrolledChild(heading.get());
    rows_.push_back({std::move(heading), 40});
    for (const auto &p : document.Parameters())
      if (editing::Section(p) == section && editing::RightColumn(p) == right)
        AddParameter(document, p);
  }
  resized();
}
void ParameterPanel::AddParameter(editing::Document &document,
                                  const editing::Parameter &p) {
  auto change = [this, &document, key = p.key](double v) {
    try {
      document.Set(key, v);
    } catch (const std::exception &e) {
      if (error)
        error(e.what());
    }
  };
  if (p.scale == 2 || p.scale == 3) {
    auto button = std::make_unique<visage::UiButton>();
    auto *widget = button.get();
    const auto label = editing::ControlName(p) + ": ";
    widget->setText(label + editing::ChoiceName(p, int(document.Value(p.key))));
    widget->onToggle() = [this, &document, p, widget, label, change](auto *,
                                                                     bool) {
      visage::PopupMenu menu;
      for (int value = int(p.minimum); value <= int(p.maximum); ++value)
        menu.addOption(value, editing::ChoiceName(p, value))
            .select(value == document.Value(p.key));
      menu.onSelection() = [this, p, widget, label, change](int value) {
        change(value);
        widget->setText(label + editing::ChoiceName(p, value));
        if (committed)
          committed();
      };
      menu.show(widget);
    };
    addScrolledChild(widget);
    rows_.push_back({std::move(button), 38});
  } else {
    auto slider = std::make_unique<Slider>(editing::ControlName(p), p.minimum,
                                           p.maximum, p.initial, " " + p.unit);
    slider->Set(document.Value(p.key));
    slider->position = [p](double v) { return editing::Position(p, v); };
    slider->valueAt = [p](double v) { return editing::ValueAt(p, v); };
    slider->changed = change;
    slider->committed = [this] {
      if (committed)
        committed();
    };
    addScrolledChild(slider.get());
    rows_.push_back({std::move(slider), 48});
  }
}
void ParameterPanel::resized() {
  visage::ScrollableFrame::resized();
  int y = 0;
  for (auto &row : rows_) {
    row.frame->setBounds(0, y, std::max(1.f, width() - 14), row.height - 4);
    y += row.height;
  }
  setScrollableHeight(float(y));
}
} // namespace drumfoundry::ui
