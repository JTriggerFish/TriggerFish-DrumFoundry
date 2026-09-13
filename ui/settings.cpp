#include "settings.hpp"
#include <stdexcept>

namespace drumfoundry::ui {
void ValidateDeviceConfiguration(const DeviceConfiguration &config) {
  if (config.api.empty() || config.device.empty())
    throw std::invalid_argument("Choose an audio API and output device first");
  if (config.rate < 8000 || config.rate > 384000 || config.buffer < 16 ||
      config.buffer > 16384)
    throw std::invalid_argument("Unsupported sample rate or buffer size");
  if (config.midi.empty())
    throw std::invalid_argument("Choose a MIDI input, all or none");
}
SettingsPanel::SettingsPanel(SettingsBridge bridge, DeviceConfiguration config)
    : bridge_(std::move(bridge)), config_(std::move(config)) {
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
    bridge_.apply(config_);
    message_ =
        "Audio started. Actual buffer and latency appear in the status bar.";
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
  redraw();
}
void SettingsPanel::Choose(visage::UiButton &button,
                           std::vector<std::string> names,
                           std::function<void(std::string)> select) {
  if (names.empty()) {
    message_ = "No matching devices found.";
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
}
void SettingsPanel::draw(visage::Canvas &c) {
  c.setColor(0xff1b2430);
  c.roundedRectangle(0, 0, width(), height(), 8);
  Label(c, "AUDIO / MIDI SETTINGS", 24, 12, width() - 140, 30, 0xffe8b755);
  unsigned row = 0;
  for (const auto *label : {"Audio API", "Output device", "MIDI input",
                            "Sample rate", "Buffer size"})
    Label(c, label, 24, 62 + 46 * row++, 118, 32);
  Label(c,
        "ASIO devices open only on Apply. Close other exclusive users first.",
        24, 354, width() - 48, 24);
  Label(c, message_, 24, 382, width() - 48, 44, 0xffefb178);
}
} // namespace drumfoundry::ui
