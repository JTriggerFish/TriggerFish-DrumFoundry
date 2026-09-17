#pragma once
#include "document.hpp"

namespace drumfoundry {
// Optional state interactions are attachments, never post-output audio effects.
inline constexpr const char *RimContactId = "rim-contact";
inline constexpr const char *RimContactType = "interaction.rim-contact";
bool HasRimContact(const Json &patch);
const char *ResonatorId(detail::Recipe);
Json RimContactNode();
Json RimContactAttachment(detail::Recipe);
void UpgradeRimContact(Json &patch);
void ValidateAttachments(const Json &patch, detail::Recipe);
} // namespace drumfoundry
