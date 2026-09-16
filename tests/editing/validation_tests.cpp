#include "parameters/validation.hpp"
#include "runtime/live_controls.hpp"
#include "runtime/voice.hpp"
#include <limits>
#include <stdexcept>

using namespace drumfoundry;
namespace {
void Require(bool ok) {
  if (!ok)
    throw std::runtime_error("Shared parameter validation regression");
}
void ScalarContract() {
  const ParameterDescriptor d{"x", "x", "", .08f, 1.f, .5f};
  Require(ValidParameterValue(d, .08));
  Require(!ValidParameterValue(d, .079));
  for (double v : {std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::quiet_NaN(), 1.e300})
    Require(!ValidParameterValue(d, v));
  Require(!ValidParameterValue(.5, 0, 1, ParameterScale::Boolean));
  Require(ValidParameterValue(1, 0, 1, ParameterScale::Boolean));
}
void DecayContract() {
  auto values = DefaultCrashMacros();
  const auto frequency = std::size_t(CrashMacro::BodyDecayFrequencyFirst);
  const auto active = std::size_t(CrashMacro::BodyDecayActiveFirst);
  values[std::size_t(CrashMacro::BodyDecayMaximumFrequency)] = 15000;
  values[frequency] = 18000;
  values[active] = 0;
  Require(ValidLiveDecay(values));
  values[active] = 1;
  Require(!ValidLiveDecay(values));

  auto patch = DefaultPatch("metal.cymbal.v1");
  for (auto &node : patch["nodes"]) {
    auto &p = node["parameters"];
    if (!p.contains("body_decay_frequency_7"))
      continue;
    p["body_decay_frequency_7"] = 15000;
    p["body_decay_frequency_1"] = 18000;
    p["body_decay_active_1"] = 1;
    bool rejected = false;
    try {
      ValidateDecayParameters(p);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    Require(rejected);
    p["body_decay_active_1"] = 0;
    ValidateDecayParameters(p);
  }
}
} // namespace
int main() {
  ScalarContract();
  DecayContract();
}
