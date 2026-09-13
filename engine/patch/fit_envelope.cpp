#include "document.hpp"
#include <stdexcept>
namespace drumfoundry {
Json WithFitEnvelope(Json document) {
  for (const char *key : {"id", "name"})
    if (document.contains(key) && !document.at(key).is_string())
      throw std::invalid_argument(std::string("Invalid document ") + key);
  if (document.at("schema") == "triggerfish.percussion.fit/v1")
    return document;
  if (document.at("schema") != "triggerfish.percussion.patch/v1")
    throw std::invalid_argument("Unsupported editable document schema");
  const Strike event;
  // Raw patches have the same explicit default strike as Voice. Presentation
  // metadata does not alter their synthesis parameters or normalize output.
  Json fit{{"schema", "triggerfish.percussion.fit/v1"},
           {"id", document.value("id", "native-patch")},
           {"name", document.value("name", "Imported patch")},
           {"renderer",
            {{"api", 1},
             {"adapter", "percussion-recipe-v1"},
             {"recipe", document.at("recipe")}}},
           {"reference", nullptr},
           {"controls",
            {{"event",
              {{"strength", event.strength},
               {"location", event.location},
               {"hardness", event.hardness},
               {"implement", event.implement},
               {"contactSpread", event.contactSpread},
               {"constraint", event.constraint},
               {"seed", event.seed}}},
             {"analysis",
              {{"size", 4096},
               {"hop", 1024},
               {"window", "hann"},
               {"floorDb", -180},
               {"dynamicRangeDb", 80}}}}}};
  if (document.at("recipe") == "drum.kick.v1")
    fit["controls"]["event"]["location"] = 0;
  fit["instrument"] = std::move(document);
  return fit;
}
} // namespace drumfoundry
