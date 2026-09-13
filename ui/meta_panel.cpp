#include "meta_panel.hpp"
#include <exception>
#include <sstream>
namespace drumfoundry::ui {
MetaPanel::MetaPanel() {
  addChild(&amount_);
  addChild(&keep_);
  addChild(&cancel_);
  keep_.setFont(Font());
  cancel_.setFont(Font());
  amount_.changed = [this](double) { Preview(); };
  amount_.committed = [this] {
    if (committed)
      committed();
  };
  keep_.onToggle() = [this](auto *, bool) { setVisible(false); };
  cancel_.onToggle() = [this](auto *, bool) {
    if (document_) {
      *document_ = baseline_;
      if (changed)
        changed();
      if (committed)
        committed();
    }
    setVisible(false);
  };
}
void MetaPanel::Open(editing::Document &document, bool size) {
  document_ = &document;
  baseline_ = document;
  size_ = size;
  amount_.SetLabel(size ? "Size meta" : "Earlier  —  Later");
  amount_.Set(.5);
  status_ = "Move the slider to preview visible parameter changes.";
  setVisible(true);
}
void MetaPanel::Preview() {
  if (!document_)
    return;
  try {
    const auto edit =
        size_ ? editing::SizeMeta(baseline_, amount_.Value())
              : editing::BloomTiming(baseline_, 2 * amount_.Value() - 1);
    document_->SetMany(edit.values);
    std::ostringstream text;
    if (size_)
      text << edit.values.size()
           << " visible controls updated. This replaces the starting shape.";
    else {
      for (const auto &[key, value] : edit.values)
        text << editing::ControlName(document_->Description(key)) << ": "
             << value << "  ";
    }
    if (!edit.limited.empty())
      text << " | " << edit.limited.size() << " controls reached their limits";
    status_ = text.str();
    if (changed)
      changed();
    redraw();
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void MetaPanel::resized() {
  amount_.setBounds(20, 94, width() - 40, 44);
  cancel_.setBounds(width() - 208, height() - 48, 88, 28);
  keep_.setBounds(width() - 108, height() - 48, 88, 28);
}
void MetaPanel::draw(visage::Canvas &c) {
  c.setColor(0xff17202a);
  c.roundedRectangle(0, 0, width(), height(), 8);
  Label(c, size_ ? "Size meta" : "Bloom timing", 20, 12, width() - 40, 28,
        0xffe8b755);
  Label(c,
        size_ ? "A starting-point tool; changes modes, excitation, texture and "
                "damping."
              : "Moves diffusion rate and the initial excitation distribution "
                "together.",
        20, 48, width() - 40, 22);
  Label(
      c,
      "Ordinary controls update below. Release to apply; Cancel restores them.",
      20, 70, width() - 40, 22);
  Label(c, status_, 20, 150, width() - 40, 65);
}
} // namespace drumfoundry::ui
