#include "ui/theme.hpp"
#include "ui/typography.hpp"
#include <cmath>
#include <fstream>
#include <random>
#include <stdexcept>
#include <visage/widgets.h>

namespace {
void Check(bool ok) {
  if (!ok)
    throw std::runtime_error("Colour scheme regression");
}
unsigned Colour(const visage::Frame &frame, visage::theme::ColorId id) {
  return frame.paletteColor(id).gradient().colors().front().toARGB();
}
double Luminance(unsigned colour) {
  const auto linear = [](unsigned channel) {
    const double x = channel / 255.;
    return x <= .04045 ? x / 12.92 : std::pow((x + .055) / 1.055, 2.4);
  };
  return .2126 * linear((colour >> 16) & 255) +
         .7152 * linear((colour >> 8) & 255) + .0722 * linear(colour & 255);
}
double Contrast(unsigned text, unsigned background) {
  return (Luminance(text) + .05) / (Luminance(background) + .05);
}
} // namespace
void ThemeTests() {
  using namespace drumfoundry::ui;
  auto theme = DefaultTheme();
  ValidateTheme(theme);
  visage::Palette a, b;
  visage::Frame first, second;
  first.setPalette(&a);
  second.setPalette(&b);
  ApplyTheme(a, theme);
  ApplyTheme(b, theme);
  ConfigureTextSize(a, 2);
  const auto background = Colour(first, colours::Background);
  // Normal labels and secondary readouts remain readable on every main
  // surface.
  for (auto surface : {colours::Background, colours::Panel, colours::Raised,
                       colours::Button})
    for (auto label : {colours::Text, colours::Muted})
      Check(Contrast(Colour(first, label), Colour(first, surface)) >= 4.5);
  Check(Contrast(Colour(first, colours::SelectedText),
                 Colour(first, colours::Selected)) >= 4.5);
  theme["colors"]["Background"] = "#123456";
  theme["colors"]["Button"] = "#12345680";
  ApplyTheme(a, theme);
  Check(Colour(first, colours::Background) == 0xff123456);
  Check(Colour(second, colours::Background) == background);
  Check(Colour(first, visage::UiButton::UiButtonBackground) == 0x80123456);
  Check(first.paletteValue(TextScale) == TextSizeScale(2));
  for (int failure = 0; failure < 7; ++failure) {
    auto bad = theme;
    switch (failure) {
    case 0:
      bad["schema"] = "unknown";
      break;
    case 1:
      bad["colors"].erase("Text");
      break;
    case 2:
      bad["colors"]["Typo"] = "#000000";
      break;
    case 3:
      bad["colors"]["Text"] = "#GGHHII";
      break;
    case 4:
      bad["colors"]["Text"] = 123;
      break;
    case 5:
      bad["colors"]["Text"] = "#12345";
      break;
    default:
      bad["name"] = "";
      break;
    }
    bool rejected = false;
    try {
      ApplyTheme(a, bad);
    } catch (const std::exception &) {
      rejected = true;
    }
    Check(rejected && Colour(first, colours::Background) == 0xff123456);
  }
  ApplyTheme(a, DefaultTheme());
  Check(Colour(first, colours::Background) == background);
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("drumfoundry-theme-test-" + std::to_string(std::random_device{}()));
  Check(std::filesystem::create_directory(directory));
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove(path / "theme.json", ignored);
      std::filesystem::remove(path, ignored);
    }
  } cleanup{directory};
  const auto file = directory / "theme.json";
  SaveTheme(theme, file);
  Check(ReadTheme(file) == theme);
  SaveTheme(DefaultTheme(), file);
  Check(ReadTheme(file) == DefaultTheme());
  auto invalid = theme;
  invalid["colors"]["Text"] = "invalid";
  try {
    SaveTheme(invalid, file);
  } catch (const std::exception &) {
  }
  Check(ReadTheme(file) == DefaultTheme());
  std::ofstream(file) << "{broken";
  bool rejected = false;
  try {
    ReadTheme(file);
  } catch (const std::exception &) {
    rejected = true;
  }
  Check(rejected);
}
