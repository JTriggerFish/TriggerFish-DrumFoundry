#include "parameter_panel.hpp"
#include "decay_editor.hpp"
#include "eq_plot.hpp"
#include <algorithm>
#include <exception>

namespace drumfoundry::ui {
void ParameterPanel::Load(editing::Document &document, bool right) {
  generation_ = std::make_shared<int>(0);
  preview_ = nullptr;
  sliders_.clear();
  for (auto &row : rows_)
    removeScrolledChild(row.frame);
  rows_.clear();
  groups_.clear();
  std::vector<std::string> sections;
  for (const auto &p : document.Parameters()) {
    if (editing::RightColumn(p) != right)
      continue;
    const auto section = editing::Section(p);
    if (section == "Modal anchors")
      continue;
    if (std::find(sections.begin(), sections.end(), section) ==
        sections.end())
      sections.push_back(section);
  }
  for (const auto &section : sections) {
    AddGroup(section);
    if (section == "Output" &&
        (outputSpectrum || editing::HasOutputEq(document)))
      AddOutputPreview(document);
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
    if (section == "Bloom / energy travel" && holdDecay) {
      addScrolledChild(holdDecay);
      rows_.emplace_back(*holdDecay, 94);
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
    // Keep the shared EQ's scalar counterparts in the same order for every
    // recipe, regardless of its internal parameter-enum ordering.
    if (section == "Output" && editing::HasOutputEq(document)) {
      for (const auto &p : document.Parameters())
        if (editing::Section(p) == section &&
            editing::RightColumn(p) == right &&
            p.key.rfind("output_", 0) != 0)
          AddParameter(document, p);
      for (const auto *key :
           {"output_eq_enabled", "output_low_cut", "output_colour_frequency",
            "output_colour_gain", "output_high_cut"})
        AddParameter(document, document.Description(key));
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
      RefreshSpectrum();
    } catch (const std::exception &e) {
      if (error)
        error(e.what());
    }
  };
  if (p.scale == 2 || p.scale == 3) {
    auto button = std::make_unique<HelpButton>();
    button->help = ParameterHelp(p.key);
    auto *widget = button.get();
    const auto label = editing::ControlName(p) + ": ";
    widget->setText(label +
                    editing::ChoiceName(p, int(document.Value(p.key))));
    if (p.scale == 2)
      widget->setActionButton(document.Value(p.key) >= .5);
    const std::weak_ptr<int> generation = generation_;
    widget->onToggle() = [this, &document, p, widget, label, change,
                          generation](auto *, bool) {
      if (p.scale == 2) {
        const int value = document.Value(p.key) < .5 ? 1 : 0;
        change(value);
        widget->setText(label +
                        editing::ChoiceName(p, int(document.Value(p.key))));
        widget->setActionButton(document.Value(p.key) >= .5);
        if (committed)
          committed();
        return;
      }
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
    auto slider =
        std::make_unique<Slider>(editing::ControlName(p), p.minimum,
                                 p.maximum, p.initial, " " + p.unit);
    slider->Set(document.Value(p.key));
    slider->help =
        ParameterHelp(p.key) + " " + slider->help +
        " Release to update the live voice; paused drags update the "
        "offline preview.";
    slider->position = [p](double v) { return editing::Position(p, v); };
    slider->valueAt = [p](double v) { return editing::ValueAt(p, v); };
    slider->changed = change;
    slider->committed = [this] {
      if (committed)
        committed();
    };
    sliders_.push_back({slider.get(), p.key});
    addScrolledChild(slider.get());
    rows_.push_back({std::move(slider), 48});
  }
}
void ParameterPanel::AddOutputPreview(editing::Document &document) {
  if (editing::HasOutputEq(document)) {
    auto eq = std::make_unique<EqPlot>(document, outputSpectrum);
    eq->previewRate = previewRate;
    eq->changed = [this, &document] { SyncValues(document); };
    eq->committed = [this] {
      if (committed)
        committed();
    };
    eq->error = [this](const auto &text) {
      if (error)
        error(text);
    };
    preview_ = eq.get();
    addScrolledChild(preview_);
    rows_.emplace_back(std::move(eq), 190);
  } else {
    preview_ = outputSpectrum;
    addScrolledChild(preview_);
    rows_.emplace_back(*outputSpectrum, 164);
  }
}
void ParameterPanel::SyncValues(editing::Document &document) {
  for (const auto &[slider, key] : sliders_)
    slider->Set(document.Value(key));
}
} // namespace drumfoundry::ui
