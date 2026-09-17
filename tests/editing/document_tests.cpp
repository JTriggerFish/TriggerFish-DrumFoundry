#include "editing/document.hpp"
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

using namespace drumfoundry::editing;
void Require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  Require(argc == 2, "Expected preset directory");
  extern void ModuleTests();
  ModuleTests();
  for (const auto *name :
       {"kick", "snare", "hihat", "crash", "ride", "gong"}) {
    std::ifstream input(std::string(argv[1]) + "/" + name + ".fit.json");
    Document document;
    document.Load(Json::parse(input));
    Require(document.JsonValue().at("reference").is_null(),
            "Factory preset contains a reference");
    extern void RouteTests(Document);
    RouteTests(document);
    const auto original = document.JsonValue();
    Require(!document.Parameters().empty(), "Missing metadata");
    for (const auto &p : document.Parameters()) {
      const auto value = document.Value(p.key);
      document.Set(p.key, value);
      Require(document.Value(p.key) == value,
              "Lossy value round trip: " + p.key);
      Require(!Section(p).empty(), "Missing control grouping");
      if (p.key == "body_decay_friction")
        Require(Section(p) == "Modal T60" && !RightColumn(p) &&
                    std::abs(ValueAt(p, .5) - .125) < 1.e-12,
                "Tail damping belongs by the T60 curve with a fine low taper");
      if (p.owner == "observation" && p.key.rfind("contact_noise_", 0) == 0)
        Require(Section(p) == "Strike accent" && !RightColumn(p),
                "Direct noise controls belong in the visible left strike group");
      for (double position : {0., .1, .5, .9, 1.}) {
        const double v = ValueAt(p, position);
        Require(std::isfinite(v) && float(v) >= float(p.minimum) &&
                    float(v) <= float(p.maximum),
                "Invalid taper: " + p.key);
        if (p.scale < 2)
          Require(std::abs(Position(p, v) - position) < 1e-6,
                  "Taper round trip: " + p.key);
      }
    }
    Require(document.JsonValue() == original, "Metadata edit changed preset");
    bool rejected = false;
    try {
      document.Load(Json{{"schema", "broken"}});
    } catch (...) {
      rejected = true;
    }
    Require(rejected && document.JsonValue() == original,
            "Load must be transactional");
    rejected = false;
    try {
      document.Set(document.Parameters()[0].key,
                   std::numeric_limits<double>::quiet_NaN());
    } catch (...) {
      rejected = true;
    }
    Require(rejected && document.JsonValue() == original,
            "Invalid edit changed document");
    Document copy;
    copy.Load(document.JsonValue());
    Require(copy.JsonValue() == original, "Reload changed fitted parameters");
  }
}
