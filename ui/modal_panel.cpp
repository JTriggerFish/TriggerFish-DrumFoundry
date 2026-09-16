#include "modal_panel.hpp"
#include <cmath>
namespace drumfoundry::ui {
using namespace editing;
ModalPanel::ModalPanel() {
  tool_.help = "Select & move: drag empty space to box-select handles, then "
               "drag anywhere in the yellow group or packet fill to move it. "
               "Shift adds to "
               "the selection or makes a drag finer. Ctrl-scroll changes all "
               "selected sideband widths; plain scroll changes the active "
               "handle. Delete removes the selection. Double-click empty "
               "space to add a mode, or a circular handle to delete it. "
               "Paint modes adds modes by dragging instead.";
  for (auto *frame : std::initializer_list<visage::Frame *>{
           &plot_, &series_, &tool_, &clear_, &remove_, &generate_, &guide_,
           &snap_, &brush_, &guidePitch_, &frequency_, &level_, &noisiness_,
           &allocation_})
    addChild(frame);
  plot_.changed = [this] { Sync(); };
  plot_.committed = [this] {
    if (committed)
      committed();
  };
  plot_.error = series_.error = [this](const auto &text) {
    if (error)
      error(text);
  };
  series_.committed = [this] {
    Refresh();
    if (committed)
      committed();
  };
  tool_.onToggle() = [this](auto *, bool) {
    visage::PopupMenu menu;
    menu.addOption(0, "Select & move");
    menu.addOption(1, "Prominence brush");
    menu.addOption(2, "Paint modes");
    menu.onSelection() = [this](int value) {
      plot_.tool = ModalPlot::Tool(value);
      tool_.setText(value == 0   ? "Select & move"
                    : value == 1 ? "Prominence brush"
                                 : "Paint modes");
    };
    menu.show(&tool_);
  };
  clear_.onToggle() = [this](auto *, bool) {
    if (!document_)
      return;
    ReplaceModes(*document_, {});
    Refresh();
    if (committed)
      committed();
  };
  remove_.onToggle() = [this](auto *, bool) { plot_.Remove(); };
  generate_.onToggle() = [this](auto *, bool) {
    showSeries_ = !showSeries_;
    resized();
    if (layoutChanged)
      layoutChanged();
  };
  guide_.onToggle() = [this](auto *, bool) {
    plot_.guide = !plot_.guide;
    guide_.setText(plot_.guide ? "Harmonic guide ON" : "Harmonic guide OFF");
    plot_.redraw();
  };
  snap_.onToggle() = [this](auto *, bool) {
    plot_.snap = !plot_.snap;
    snap_.setText(plot_.snap ? "Snap ON" : "Snap OFF");
  };
  brush_.changed = [this](double v) { plot_.brush = v; };
  guidePitch_.changed = [this](double v) {
    plot_.base = v;
    plot_.redraw();
  };
  guidePitch_.position = [](double f) {
    return std::log(f / 8) / std::log(1000.);
  };
  guidePitch_.valueAt = [](double p) { return 8 * std::pow(1000., p); };
  for (auto *slider : {&frequency_, &level_, &noisiness_, &allocation_}) {
    slider->changed = [this](double) { EditSelection(); };
    slider->committed = [this] {
      if (committed)
        committed();
    };
  }
}
void ModalPanel::Load(Document &d) {
  document_ = &d;
  available_ = !ModePrefix(d).empty();
  if (available_) {
    const auto &p = d.Description(ModePrefix(d) + "frequency_0");
    const double low = p.minimum, high = p.maximum;
    frequency_.SetRange(low, high);
    frequency_.position = [low, high](double f) {
      return std::log(f / low) / std::log(high / low);
    };
    frequency_.valueAt = [low, high](double v) {
      return low * std::pow(high / low, v);
    };
  }
  plot_.Load(d);
  series_.Load(d);
  resized();
  Sync();
}
void ModalPanel::Refresh() { plot_.Refresh(); }
void ModalPanel::Sync() {
  const bool selected = document_ && plot_.selected >= 0;
  const bool packets = document_ && ModePrefix(*document_) == "resolved_";
  if (selected) {
    const auto m = Modes(*document_).at(plot_.selected);
    frequency_.Set(m.frequency);
    level_.Set(m.level);
    noisiness_.Set(m.turbulence);
    allocation_.Set(m.allocation);
  }
  for (auto *slider : {&frequency_, &level_, &noisiness_, &allocation_}) {
    const bool active =
        selected && (packets || slider == &frequency_ || slider == &level_);
    slider->setIgnoresMouseEvents(!active, false);
    slider->setAlphaTransparency(active ? 1.f : .4f);
  }
  remove_.setActive(selected);
  const auto count = plot_.SelectionCount();
  frequency_.SetLabel(count > 1 ? "Active centre frequency"
                                : "Centre frequency");
  level_.SetLabel(count > 1 ? "Active prominence" : "Prominence");
  noisiness_.SetLabel(count > 1 ? "Active local noisiness" : "Local noisiness");
  allocation_.SetLabel(count > 1 ? "Active sideband allocation"
                                 : "Sideband allocation");
  remove_.setText(count > 1 ? "Delete " + std::to_string(count) + " modes"
                            : "Delete mode");
  remove_.setAlphaTransparency(selected ? 1.f : .4f);
  redraw();
}
void ModalPanel::EditSelection() {
  if (!document_ || plot_.selected < 0)
    return;
  try {
    SetMode(*document_, unsigned(plot_.selected),
            {frequency_.Value(), level_.Value(), noisiness_.Value(),
             allocation_.Value()});
    plot_.Refresh();
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
    Sync();
  }
}
} // namespace drumfoundry::ui
