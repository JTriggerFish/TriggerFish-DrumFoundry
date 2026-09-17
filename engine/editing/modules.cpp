#include "document.hpp"
#include "patch/modules.hpp"
#include <algorithm>
#include <stdexcept>

namespace drumfoundry::editing {
void Document::SetModule(const std::string &type, bool present) {
  if (type != RimContactType)
    throw std::invalid_argument("Unsupported optional module: " + type);
  auto next = document_;
  auto &patch = Instrument(next);
  if (HasRimContact(patch) == present) return;
  if (present) {
    auto module = RimContactNode();
    module["parameters"]["hat_contact_enabled"] = 1;
    patch["nodes"].push_back(std::move(module));
    patch["attachments"] = Json::array({RimContactAttachment(ParseRecipe(recipe_))});
  } else {
    auto &nodes = patch.at("nodes");
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const auto &node) {
      return node.at("id") == RimContactId;
    }), nodes.end());
    patch.erase("attachments");
  }
  Load(std::move(next)); // Transactional validation and authoritative defaults.
}
} // namespace drumfoundry::editing
