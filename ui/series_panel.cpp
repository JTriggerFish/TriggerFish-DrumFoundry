#include "series_panel.hpp"
#include <algorithm>
#include <cmath>
namespace drumfoundry::ui {
using namespace editing;
SeriesPanel::SeriesPanel() {
  addChild(&family_);
  addChild(&note_);
  addChild(&replace_);
  const char *labels[]{"Base pitch (Hz)",    "Mode count",
                       "Upper-mode stretch", "Protected low modes",
                       "Falloff (dB/oct)",   "Top level (dB)",
                       "Noisiness response"};
  const double low[]{8, 1, 0, 1, -12, -60, 0},
      high[]{8000, 32, 1, 8, 24, 6, 2}, initial[]{55, 16, 0, 4, 6, 0, 1};
  for (int i = 0; i < 7; ++i) {
    auto slider =
        std::make_unique<Slider>(labels[i], low[i], high[i], initial[i]);
    if (i == 0) {
      slider->position = [](double v) {
        return std::log(v / 8) / std::log(1000.);
      };
      slider->valueAt = [](double p) { return 8 * std::pow(1000., p); };
    } else if (i == 1 || i == 3) {
      slider->integer = true;
      slider->valueAt = [a = low[i], b = high[i]](double p) {
        return std::round(a + p * (b - a));
      };
    }
    slider->changed = [this](double) { Preview(); };
    addChild(slider.get());
    fields_.push_back(std::move(slider));
  }
  family_.onToggle() = [this](auto *, bool) {
    visage::PopupMenu menu;
    menu.addOption(0, "Harmonic").select(!membrane_);
    menu.addOption(1, "Membrane").select(membrane_);
    menu.onSelection() = [this](int choice) {
      membrane_ = choice == 1;
      family_.setText(membrane_ ? "Membrane" : "Harmonic");
      Preview();
    };
    menu.show(&family_);
  };
  note_.onToggle() = [this](auto *, bool) {
    visage::PopupMenu menu;
    constexpr const char *names[]{"C",  "C#", "D",  "D#", "E",  "F",
                                  "F#", "G",  "G#", "A",  "A#", "B"};
    for (int midi = 12; midi <= 119; ++midi)
      menu.addOption(midi, std::string(names[midi % 12]) +
                               std::to_string(midi / 12 - 1));
    menu.onSelection() = [this](int midi) {
      fields_[0]->Set(440 * std::exp2((midi - 69.) / 12));
      Preview();
    };
    menu.show(&note_);
  };
  replace_.onToggle() = [this](auto *, bool) { Apply(); };
}
void SeriesPanel::Load(Document &d) {
  document_ = &d;
  Preview();
}
Series SeriesPanel::Settings() const {
  return {membrane_ ? SeriesFamily::Membrane : SeriesFamily::Harmonic,
          fields_[0]->Value(),
          fields_[2]->Value(),
          fields_[5]->Value(),
          fields_[4]->Value(),
          fields_[6]->Value(),
          unsigned(fields_[1]->Value()),
          unsigned(fields_[3]->Value())};
}
void SeriesPanel::Preview() {
  if (!document_)
    return;
  try {
    const auto prefix = ModePrefix(*document_);
    const auto &p = document_->Description(prefix + "frequency_0");
    const auto modes = GenerateSeries(Settings(), p.minimum, p.maximum,
                                      unsigned(Modes(*document_).size()));
    status_ =
        std::to_string(modes.size()) + " modes — damping and bloom unchanged";
    replace_.setActive(true);
    replace_.setAlphaTransparency(1);
  } catch (const std::exception &e) {
    status_ = e.what();
    replace_.setActive(false);
    replace_.setAlphaTransparency(.4f);
  }
  redraw();
}
void SeriesPanel::Apply() {
  if (!document_)
    return;
  try {
    const auto &p =
        document_->Description(ModePrefix(*document_) + "frequency_0");
    ReplaceModes(*document_,
                 GenerateSeries(Settings(), p.minimum, p.maximum,
                                unsigned(Modes(*document_).size())));
    if (committed)
      committed();
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
  }
}
void SeriesPanel::resized() {
  family_.setBounds(0, 0, 120, 28);
  note_.setBounds(128, 0, 100, 28);
  replace_.setBounds(width() - 144, 0, 144, 28);
  const unsigned columns = width() < 560 ? 2 : 3;
  const float col = (width() - 12 * (columns - 1)) / columns;
  for (unsigned i = 0; i < fields_.size(); ++i)
    fields_[i]->setBounds((col + 12) * (i % columns), 36 + 48 * (i / columns),
                          col, 44);
}
void SeriesPanel::draw(visage::Canvas &c) {
  Label(c, status_, 0, height() - 24, width(), 22, colours::Heading);
}
} // namespace drumfoundry::ui
