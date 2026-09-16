#include "ui/controls.hpp"
#include "ui/help_bubble.hpp"
#include "ui/settings.hpp"
#include "ui/split_bar.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

void Require(bool condition) {
  if (!condition)
    throw std::runtime_error("UI control regression");
}
void ThemeTests();
int main() {
  ThemeTests();
  using namespace drumfoundry::ui;
  Require(ElideText(Font(17), "Level", 200) == "Level");
  const auto caption = ElideText(Font(17), "Long parameter caption", 70);
  Require(caption != "Long parameter caption" &&
          Font(17).stringWidth(visage::String(caption).toUtf32()) <= 70);
  Require(ElideText(Font(17), "Level", 0).empty());
  visage::Palette textPalette;
  Slider frequency("Colour frequency", 40, 20000, 7200, " Hz");
  frequency.setPalette(&textPalette);
  for (int size : {0, 1, 2}) {
    ConfigureTextSize(textPalette, size);
    Require(frequency.PreferredHeight(530) == 44);
    Require(frequency.PreferredHeight(100) == 66);
    Require(frequency.PreferredHeight(530) == 44); // Widening unwraps again.
  }
  Slider slider("Test", -60, 0, -12);
  frequency.SetRange(15000, 20000);
  frequency.Set(18000);
  Require(frequency.Value() == 18000 && !frequency.SubmitText("14000"));
  frequency.SetRange(40, 15000);
  Require(frequency.Value() == 15000 && !frequency.SubmitText("18000"));
  slider.setBounds(0, 0, 212, 44);
  slider.Set(100);
  Require(slider.Value() == 0);
  slider.Set(std::numeric_limits<double>::quiet_NaN());
  Require(slider.Value() == 0);
  double notified = 999;
  slider.changed = [&](double value) { notified = value; };
  visage::MouseEvent event;
  event.button_id = visage::kMouseButtonLeft;
  HelpBubble help;
  help.Bind(slider);
  help.Bind(slider); // Rebuild traversal must not duplicate handlers.
  event.repeat_click_count = 2;
  slider.processMouseDown(event);
  Require(slider.Value() == -12); // Native input survived help binding.
  slider.processMouseUp(event);
  Require(!ParameterHelp("field_packet_spread").empty());
  event.repeat_click_count = 1;
  event.repeat_click_count = 2;
  slider.mouseDown(event);
  Require(notified == -12);
  event.repeat_click_count = 1;
  event.position.x = 106;
  slider.mouseDown(event);
  Require(slider.Value() == -30);
  event.modifiers = visage::kModifierShift;
  slider.mouseDown(event);
  event.position.x += 100;
  slider.mouseDrag(event);
  Require(std::abs(slider.Value() + 27) < 1e-9);
  slider.mouseUp(event);
  event.position.x = 0;
  slider.mouseDrag(event);
  Require(std::abs(slider.Value() + 27) < 1e-9);
  unsigned entries = 0, rejected = 0;
  slider.committed = [&] { ++entries; };
  slider.error = [&](const auto &) { ++rejected; };
  Require(slider.SubmitText(" -1.25e1 "));
  Require(slider.Value() == -12.5 && notified == -12.5 && entries == 1);
  for (const auto *invalid : {"", "nan", "inf", "-12dB", "-61", "1", "2,3"})
    Require(!slider.SubmitText(invalid));
  Require(slider.Value() == -12.5 && entries == 1 && rejected == 7);
  slider.integer = true;
  Require(!slider.SubmitText("-2.5"));
  Require(slider.SubmitText("-3"));
  event.button_id = visage::kMouseButtonRight;
  slider.mouseDown(event);
  Require(slider.children().size() == 1);
  auto *entry = dynamic_cast<visage::TextEditor *>(slider.children().front());
  Require(entry && entry->isVisible());
  entry->setText("-7");
  entry->onEscapeKey().callback();
  Require(!entry->isVisible() && slider.Value() == -3);
  slider.mouseDown(event);
  entry->setText("-8");
  entry->onEnterKey().callback();
  Require(!entry->isVisible() && slider.Value() == -8);
  event.button_id = visage::kMouseButtonLeft;
  SplitBar divider;
  float movement = 0;
  divider.dragged = [&](float delta) {
    movement = delta;
    divider.setBounds(0, delta, 200, 10);
  };
  event.window_position.y = 100;
  divider.mouseDown(event);
  event.window_position.y = 125;
  divider.mouseDrag(event);
  Require(movement == 25);
  event.window_position.y = 100;
  divider.mouseDrag(event);
  Require(movement == 0);
  divider.mouseUp(event);
  divider.vertical = true;
  event.window_position.x = 40;
  divider.mouseDown(event);
  event.window_position.x = 80;
  divider.mouseDrag(event);
  Require(movement == 40);
  event.window_position.x = 40;
  divider.mouseDrag(event);
  Require(movement == 0);
  divider.mouseUp(event);
  StrikePad pad;
  pad.setBounds(0, 0, 200, 100);
  float velocity = 0, location = 0;
  pad.strike = [&](float v, float x) {
    velocity = v;
    location = x;
  };
  const auto area = pad.PlayingBounds();
  event.position = {area.x() + area.width() * .25f,
                    area.y() + area.height() * .25f};
  pad.mouseDown(event);
  Require(velocity == .75f && location == .25f);
  for (int kind : {0, 1, 2}) {
    pad.SetKick(kind == 0);
    pad.SetMembrane(kind == 1);
    for (auto point :
         {area.topLeft(), visage::Point{area.right(), area.bottom()},
          visage::Point{area.x() + area.width() / 2,
                        area.y() + area.height() / 2},
          visage::Point{0, 0}, visage::Point{200, 100}}) {
      event.position = point;
      pad.mouseDown(event);
      Require(
          velocity ==
          std::clamp(1.f - (point.y - area.y()) / area.height(), .01f, 1.f));
      Require(location ==
              std::clamp((point.x - area.x()) / area.width(), 0.f, 1.f));
    }
  }
  unsigned applies = 0, errors = 0;
  SettingsBridge settings;
  settings.apply = [&](const DeviceConfiguration &) {
    ++applies;
    return std::string{};
  };
  SettingsPanel invalid(settings, {"test", "", "all", 48000, 128});
  invalid.error = [&](const std::string &) { ++errors; };
  Require(!invalid.Apply());
  Require(applies == 0 && errors == 1);
  SettingsPanel valid(settings, {"test", "device", "all", 48000, 128});
  Require(valid.Apply());
  Require(applies == 1);
  settings.apply = [&](const DeviceConfiguration &) {
    ++applies;
    return std::string("Audio running. MIDI unavailable");
  };
  SettingsPanel partial(settings, {"test", "device", "all", 48000, 128});
  partial.error = [&](const std::string &text) {
    Require(text == "Audio running. MIDI unavailable");
    ++errors;
  };
  Require(partial.Apply());
  Require(applies == 2 && errors == 2);
  settings.apply = [&](const DeviceConfiguration &) -> std::string {
    throw std::runtime_error("Device busy");
  };
  SettingsPanel busy(settings, {"test", "device", "all", 48000, 128});
  busy.error = [&](const std::string &text) {
    Require(text == "Device busy");
    ++errors;
  };
  Require(!busy.Apply() && errors == 3);
  return 0;
}
