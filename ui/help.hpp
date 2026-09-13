#pragma once
#include <string>
#include <visage/widgets.h>
namespace drumfoundry::ui {
// Help metadata only. Standard controls retain their native input/drawing.
struct HelpText {
  virtual ~HelpText() = default;
  std::string help;
  bool helpBound{};
};
class HelpButton : public visage::UiButton, public HelpText {
public:
  using visage::UiButton::UiButton;
};
std::string ParameterHelp(const std::string &key);
} // namespace drumfoundry::ui
