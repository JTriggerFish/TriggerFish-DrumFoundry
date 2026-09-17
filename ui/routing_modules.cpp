#include "routing_panel.hpp"
#include "patch/modules.hpp"

namespace drumfoundry::ui {
void RoutingPanel::SetModule(const std::string &type, bool present) {
  try {
    if (!document_) return;
    document_->SetModule(type, present);
    Load(*document_);
    if (changed) changed();
  } catch (const std::exception &e) {
    if (error) error(e.what());
  }
}
void RoutingPanel::ModulesMenu() {
  if (!document_) return;
  const auto &root = document_->JsonValue();
  const auto &patch = root.contains("instrument") ? root.at("instrument") : root;
  const bool present = HasRimContact(patch);
  visage::PopupMenu menu;
  if (!present) menu.addOption(1, "Add rim contact to resonator");
  else {
    menu.addOption(2, "Rim contact enabled")
        .select(document_->Value("hat_contact_enabled") >= .5);
    menu.addOption(3, "Remove rim contact");
  }
  menu.onSelection() = [this](int choice) {
    try {
      if (choice == 2)
        document_->Set("hat_contact_enabled", document_->Value("hat_contact_enabled") < .5);
      else if (choice == 1 || choice == 3) {
        SetModule(RimContactType, choice == 1);
        return;
      }
      else return;
      Load(*document_);
      if (changed) changed();
    } catch (const std::exception &e) {
      if (error) error(e.what());
    }
  };
  menu.show(&modules_);
}
} // namespace drumfoundry::ui
