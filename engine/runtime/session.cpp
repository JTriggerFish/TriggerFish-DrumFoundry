#include "session.hpp"

#include <array>
#include <cmath>

namespace drumfoundry::detail {
void Initialize(Session &session, const Recipe recipe,
                const float sampleRate) noexcept {
  session.recipe = recipe;
  session.sampleRate = sampleRate;
  session.crashBase = MetallicBaseFit();
  session.crashValues = DefaultCrashMacros();
  session.kickValues = DefaultKickParameters();
  session.membraneValues = DefaultMembraneParameters();
  session.snareValues = DefaultSnareParameters();
  session.cymbalRouting = {};
  session.kickRouting = {};
  session.membraneRouting = {};
  session.snareRouting = {};
}

const ParameterDescriptor *Description(const Session &session,
                                       const std::size_t index) noexcept {
  return Description(session.recipe, index);
}
const ParameterDescriptor *Description(Recipe recipe,
                                       std::size_t index) noexcept {
  switch (recipe) {
  case Recipe::MetallicPlate:
    return index < ActiveCrashMacroCount ? &ActiveCrashMacroDescription(index)
                                         : nullptr;
  case Recipe::Kick:
    return index < KickParameterValues{}.size()
               ? &KickParameterDescription(index)
               : nullptr;
  case Recipe::MembraneDrum:
    return index < MembraneParameterValues{}.size()
               ? &MembraneParameterDescription(index)
               : nullptr;
  case Recipe::SnareDrum:
    return index < SnareParameterValues{}.size()
               ? &SnareParameterDescription(index)
               : nullptr;
  default:
    return nullptr;
  }
}

std::size_t ParameterCount(const Session &session) noexcept {
  return ParameterCount(session.recipe);
}
std::size_t ParameterCount(Recipe recipe) noexcept {
  switch (recipe) {
  case Recipe::MetallicPlate:
    return ActiveCrashMacroCount;
  case Recipe::Kick:
    return KickParameterValues{}.size();
  case Recipe::MembraneDrum:
    return MembraneParameterValues{}.size();
  case Recipe::SnareDrum:
    return SnareParameterValues{}.size();
  default:
    return 0;
  }
}

void Prepare(Session &session) {
  if (session.recipe == Recipe::MetallicPlate) {
    auto parameters = tfdsp::percussion::DefaultCrashCymbalParameters(
        session.sampleRate,
        ApplyCrashMacros(session.crashBase, session.crashValues));
    parameters.routing = session.cymbalRouting;
    session.cymbal.Prepare(session.sampleRate, parameters);
    return;
  }
  if (session.recipe == Recipe::Kick) {
    auto parameters = ApplyKickParameters(session.kickValues);
    tfdsp::percussion::ApplyKickRouting(parameters, session.kickRouting);
    session.kick.Prepare(session.sampleRate, parameters);
    return;
  }
  if (session.recipe == Recipe::MembraneDrum) {
    auto parameters = ApplyMembraneParameters(session.membraneValues);
    parameters.routing = session.membraneRouting;
    session.membrane.Prepare(session.sampleRate, parameters);
    return;
  }
  auto parameters = ApplySnareParameters(session.snareValues);
  parameters.routing = session.snareRouting;
  session.snare.Prepare(session.sampleRate, parameters);
}

float Process(Session &session) noexcept {
  switch (session.recipe) {
  case Recipe::MetallicPlate:
    return session.cymbal.Process();
  case Recipe::Kick:
    return session.kick.Process();
  case Recipe::MembraneDrum:
    return session.membrane.Process();
  case Recipe::SnareDrum:
    return session.snare.Process();
  default:
    return 0.f;
  }
}

} // namespace drumfoundry::detail
