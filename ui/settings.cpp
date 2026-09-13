#include "settings.hpp"
#include <stdexcept>

namespace drumfoundry::ui {
SettingsPanel::SettingsPanel(SettingsBridge bridge, DeviceConfiguration config)
    : bridge_(std::move(bridge)), config_(std::move(config)) {
  addChild(&messageView_);
  messageView_.onScroll() = [this](auto *) { messageView_.redraw(); };
  messageText_.setFont(Font());
  messageText_.setMultiLine(true);
  messageText_.setJustification(visage::Font::kTopLeft);
  messageView_.onDraw() = [this](visage::Canvas &c) {
    c.setColor(0xffefb178);
    c.text(&messageText_, 0, -messageView_.yPosition(),
           messageView_.width() - 12, messageHeight_);
  };
  for (auto *button :
       {&api_, &device_, &midi_, &rate_, &buffer_, &apply_, &stop_, &close_}) {
    addChild(button);
    button->setFont(Font());
  }
  close_.onToggle() = [this](auto *, bool) { setVisible(false); };
  api_.onToggle() = [this](auto *, bool) {
    Guard([&] {
      Choose(api_, bridge_.apis(), [this](auto name) {
        config_.api = name;
        config_.device.clear();
        Refresh();
      });
    });
  };
  device_.onToggle() = [this](auto *, bool) {
    Guard([&] {
      Choose(device_, bridge_.devices(config_.api), [this](auto name) {
        config_.device = name;
        Refresh();
      });
    });
  };
  midi_.onToggle() = [this](auto *, bool) {
    Guard([&] {
      Choose(midi_, bridge_.midi(), [this](auto name) {
        config_.midi = name;
        Refresh();
      });
    });
  };
  rate_.onToggle() = [this](auto *, bool) {
    Choose(rate_, {"44100", "48000", "88200", "96000", "192000"},
           [this](auto name) {
             config_.rate = unsigned(std::stoul(name));
             Refresh();
           });
  };
  buffer_.onToggle() = [this](auto *, bool) {
    Choose(buffer_, {"32", "64", "128", "256", "512", "1024", "2048"},
           [this](auto name) {
             config_.buffer = unsigned(std::stoul(name));
             Refresh();
           });
  };
  apply_.onToggle() = [this](auto *, bool) { Apply(); };
  stop_.onToggle() = [this](auto *, bool) {
    Guard([&] {
      bridge_.stop();
      message_ = "Audio and MIDI devices released.";
    });
  };
  Refresh();
}
void SettingsPanel::Apply() {
  Guard([&] {
    ValidateDeviceConfiguration(config_);
    message_ = bridge_.apply(config_);
    if (message_.empty())
      message_ = "Audio started. Selections remembered for next session.";
    else if (error)
      error(message_);
  });
}
void SettingsPanel::Guard(const std::function<void()> &action) {
  try {
    action();
  } catch (const std::exception &e) {
    message_ = e.what();
    if (error)
      error(message_);
  }
  UpdateMessage();
  messageView_.setYPosition(0);
  redraw();
}
void SettingsPanel::UpdateMessage() {
  messageText_.setText(message_);
  const auto text = visage::String(message_).toUtf32();
  const auto lines = Font().lineBreaks(
      text.c_str(), int(text.size()), std::max(1.f, messageView_.width() - 12));
  messageHeight_ = float(lines.size() + 1) * 20;
  messageView_.setScrollableHeight(messageHeight_);
  messageView_.redraw();
}
void SettingsPanel::Choose(visage::UiButton &button,
                           std::vector<std::string> names,
                           std::function<void(std::string)> select) {
  if (names.empty()) {
    message_ = "No matching devices found.";
    UpdateMessage();
    redraw();
    return;
  }
  visage::PopupMenu menu;
  for (unsigned i = 0; i < names.size(); ++i)
    menu.addOption(i, names[i]);
  menu.onSelection() = [names = std::move(names),
                        select = std::move(select)](int index) {
    if (index >= 0 && size_t(index) < names.size())
      select(names[index]);
  };
  menu.show(&button);
}
void SettingsPanel::Refresh() {
  api_.setText(config_.api);
  device_.setText(config_.device.empty() ? "Choose device…" : config_.device);
  midi_.setText(config_.midi);
  rate_.setText(std::to_string(config_.rate) + " Hz");
  buffer_.setText(std::to_string(config_.buffer) + " samples");
}
void SettingsPanel::resized() {
  unsigned row = 0;
  for (auto *button : {&api_, &device_, &midi_, &rate_, &buffer_})
    button->setBounds(150, 62 + 46 * row++, width() - 174, 32);
  close_.setBounds(width() - 100, 14, 76, 28);
  apply_.setBounds(24, 308, 180, 32);
  stop_.setBounds(220, 308, 170, 32);
  messageView_.setBounds(24, 382, width() - 48, height() - 396);
  UpdateMessage();
}
void SettingsPanel::draw(visage::Canvas &c) {
  c.setColor(0xff1b2430);
  c.roundedRectangle(0, 0, width(), height(), 8);
  Label(c, "AUDIO / MIDI SETTINGS", 24, 12, width() - 140, 30, 0xffe8b755);
  unsigned row = 0;
  for (const auto *label : {"Audio API", "Output device", "MIDI input",
                            "Sample rate", "Buffer size"})
    Label(c, label, 24, 62 + 46 * row++, 118, 32);
  Label(c, "Selections are saved on Apply. Devices stay closed on next launch.",
        24, 354, width() - 48, 24);
}
} // namespace drumfoundry::ui
