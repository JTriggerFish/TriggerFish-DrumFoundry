#pragma once
#include "runtime/session.hpp"
#include <cstdint>
#include <nlohmann/json.hpp>

namespace drumfoundry {
using Json = nlohmann::json;
struct Strike {
  float strength{.8f}, location{.5f}, hardness{.5f}, implement{1.f};
  float contactSpread{.2f}, constraint{};
  std::uint32_t seed{1};
};
// Durable JSON contracts; all parsing and preparation run off the audio thread.
Json ParseJson(const char *text);
Json &Instrument(Json &document);
const Json &Topology(detail::Recipe recipe);
detail::Recipe ParseRecipe(const std::string &key);
void ValidateTopology(const Json &patch, detail::Recipe recipe);
void ValidateEnvelope(const Json &document);
void UpgradeOutputEq(Json &patch);
// Validate supplied values, materialize missing defaults, then apply the patch.
void ApplyPatch(detail::Session &, Json &patch);
Strike ReadStrike(const Json &event, bool fixedBeater = false);
void ValidateStrike(const Strike &);
Json DescribeParameters(const detail::Session &);
Json DefaultPatch(const std::string &recipe);
// Add explicit strike/presentation defaults when opening a raw patch in a host.
// Existing fits pass through unchanged; the core Voice still accepts raw
// patches.
Json WithFitEnvelope(Json);
} // namespace drumfoundry
