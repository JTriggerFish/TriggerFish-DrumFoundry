#include "eq_plot.hpp"
#include "parameters/validation.hpp"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace drumfoundry::ui {
namespace {
constexpr const char *keys[]{"output_low_cut", "output_colour_frequency",
                             "output_colour_gain", "output_high_cut",
                             "output_colour_q"};
constexpr const char *names[]{"HP", "Colour", "Gain", "LP", "Peak Q"};
std::string Number(double value, int precision = 5) {
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::setprecision(precision) << value;
  return stream.str();
}
} // namespace
void EqPlot::resized() {
  const float badge = 80 * paletteValue(TextScale);
  enabled_.setBounds(width() - badge - 6, 4, badge, 24);
  const float cell = (width() - 18) / 2;
  // Cuts together, then bell frequency/gain, with an always-visible Q readout.
  constexpr unsigned positions[]{0, 2, 3, 1, 4};
  for (unsigned i = 0; i < values_.size(); ++i) {
    const unsigned position = positions[i];
    values_[i].setBounds(6 + (position % 2) * (cell + 6),
                         height() - 83 + (position / 2) * 27,
                         i == 4 ? width() - 12 : cell, 24);
  }
  entry_.setBounds(values_[editing_].bounds());
}
void EqPlot::SyncReadouts() {
  const bool enabled = document_.Value("output_eq_enabled") >= .5;
  const std::array<double, 6> current{
      document_.Value(keys[0]), document_.Value(keys[1]),
      document_.Value(keys[2]), document_.Value(keys[3]),
      document_.Value(keys[4]), double(enabled)};
  if (readoutsReady_ && displayed_ == current)
    return;
  displayed_ = current;
  readoutsReady_ = true;
  enabled_.setText(enabled ? "EQ on" : "EQ off");
  enabled_.setActionButton(enabled);
  for (unsigned i = 0; i < values_.size(); ++i) {
    values_[i].setText(std::string(names[i]) + " " +
                       Number(document_.Value(keys[i])) +
                       (i == 4 ? "" : i == 2 ? " dB" : " Hz"));
    values_[i].help = ParameterHelp(keys[i]) +
                      " Click to type; Enter applies, Escape cancels.";
  }
}
void EqPlot::EditValue(unsigned index) {
  editing_ = index;
  entry_.setFont(FrameFont(*this));
  entry_.setText(Number(document_.Value(keys[index]), 12));
  resized();
  entry_.setVisible(true);
  entry_.requestKeyboardFocus();
  entry_.selectAll();
}
bool EqPlot::SubmitValue(unsigned index, const std::string &text) {
  if (index >= values_.size())
    return false;
  try {
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    double value{};
    const bool parsed = bool(stream >> value);
    stream >> std::ws;
    const auto &p = document_.Description(keys[index]);
    if (!parsed || !stream.eof() ||
        !ValidParameterValue(value, float(p.minimum), float(p.maximum),
                             static_cast<ParameterScale>(p.scale)))
      throw std::runtime_error(std::string(names[index]) + ": enter " +
                               Number(p.minimum) + " to " +
                               Number(p.maximum) + " " + p.unit);
    document_.Set(keys[index], value);
    entry_.setVisible(false);
    SyncReadouts();
    redraw();
    if (changed)
      changed();
    if (committed)
      committed();
    return true;
  } catch (const std::exception &e) {
    if (error)
      error(e.what());
    return false;
  }
}
} // namespace drumfoundry::ui
