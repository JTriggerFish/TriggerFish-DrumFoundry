#include "document.hpp"
#include <cmath>
#include <stdexcept>

namespace drumfoundry {
namespace {
void Unit(float value) {
  if (!std::isfinite(value) || value < 0 || value > 1)
    throw std::invalid_argument(
        "Strike controls must be finite and between zero and one");
}
} // namespace
void ValidateStrike(const Strike &s) {
  Unit(s.strength);
  Unit(s.location);
  Unit(s.hardness);
  Unit(s.implement);
  Unit(s.contactSpread);
  Unit(s.constraint);
}
Strike ReadStrike(const Json &event, bool fixedBeater) {
  Strike s;
  s.strength = event.at("strength").get<float>();
  s.location = fixedBeater ? 0.f : event.at("location").get<float>();
  s.hardness = event.at("hardness").get<float>();
  s.implement = event.at("implement").get<float>();
  s.contactSpread = event.at("contactSpread").get<float>();
  s.constraint = event.at("constraint").get<float>();
  const auto &seed = event.at("seed");
  if (!seed.is_number_integer() || seed.get<double>() < 0 ||
      seed.get<double>() > UINT32_MAX)
    throw std::invalid_argument("Invalid strike seed");
  s.seed = seed.get<std::uint32_t>();
  ValidateStrike(s);
  return s;
}
void ValidateEnvelope(const Json &d) {
  if (d.at("schema") == "triggerfish.percussion.patch/v1")
    return;
  if (d.at("schema") != "triggerfish.percussion.fit/v1" ||
      d.at("renderer").at("api") != 1 ||
      d.at("renderer").at("adapter") != "percussion-recipe-v1" ||
      d.at("renderer").at("recipe") != d.at("instrument").at("recipe"))
    throw std::invalid_argument("Unsupported fit format");
  const auto reference = d.value("reference", Json());
  if (!reference.is_null() &&
      (!reference.is_object() ||
       (reference.contains("id") && !reference.at("id").is_string()) ||
       (reference.contains("sha256") && !reference.at("sha256").is_string())))
    throw std::invalid_argument("Invalid reference identity");
  const auto &a = d.at("controls").at("analysis");
  const int size = a.at("size").get<int>(), hop = a.at("hop").get<int>();
  const auto window = a.at("window").get<std::string>();
  if (!a.at("size").is_number_integer() || !a.at("hop").is_number_integer() ||
      size < 2 || (size & (size - 1)) || hop < 1 || hop > size ||
      !std::isfinite(a.at("floorDb").get<double>()) ||
      !std::isfinite(a.at("dynamicRangeDb").get<double>()) ||
      (window != "hann" && window != "blackman-harris" &&
       window != "rectangular"))
    throw std::invalid_argument("Invalid analysis settings");
}
} // namespace drumfoundry
