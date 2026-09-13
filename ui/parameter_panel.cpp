#include "parameter_panel.hpp"
#include "decay_editor.hpp"
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
  generation_ = std::make_shared<int>(0);
  for (auto &row : rows_)
    removeScrolledChild(row.frame.get());
  rows_.clear();
  std::vector<std::string> sections;
  for (const auto &p : document.Parameters()) {
    if (editing::RightColumn(p) != right)
      continue;
    const auto section = editing::Section(p);
    if (section == "Modal anchors")
      continue;
    if (std::find(sections.begin(), sections.end(), section) == sections.end())
      sections.push_back(section);
  }
  for (const auto &section : sections) {
    auto heading = std::make_unique<Heading>(section);
    addScrolledChild(heading.get());
    rows_.push_back({std::move(heading), 40});
    if (meta &&
        (section == "Bloom / energy travel" ||
         (section == "Output" && document.Recipe() == "metal.cymbal.v1"))) {
      const bool size = section == "Output";
      auto tool = std::make_unique<visage::UiButton>(size ? "Size meta…"
                                                          : "Bloom timing…");
      tool->setFont(Font());
      tool->onToggle() = [this, size](auto *, bool) {
        if (meta)
          meta(size);
      };
      addScrolledChild(tool.get());
      rows_.push_back({std::move(tool), 38});
    }
    if (section == "Modal T60") {
      auto editor = std::make_unique<DecayEditor>(document);
      editor->committed = [this] {
        if (committed)
          committed();
      };
      editor->error = [this](const auto &text) {
        if (error)
          error(text);
      };
      addScrolledChild(editor.get());
      rows_.push_back({std::move(editor), 366});
      continue;
    }
    for (const auto &p : document.Parameters())
      if (editing::Section(p) == section && editing::RightColumn(p) == right)
        AddParameter(document, p);
  }
  resized();
  NativeFonts(*this);
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
    const std::weak_ptr<int> generation = generation_;
    widget->onToggle() = [this, &document, p, widget, label, change,
                          generation](auto *, bool) {
      visage::PopupMenu menu;
      for (int value = int(p.minimum); value <= int(p.maximum); ++value)
        menu.addOption(value, editing::ChoiceName(p, value))
            .select(value == document.Value(p.key));
      menu.onSelection() = [this, p, widget, label, change,
                            generation](int value) {
        if (generation.expired())
          return; // Preset changed while the menu was open.
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
