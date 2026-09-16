#include "modal_edit.hpp"
#include "live_controls.hpp"
#include "voice.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace drumfoundry {
std::unique_ptr<PreparedModalEdit> PrepareModalEdit(float rate, Json next) {
  if (!std::isfinite(rate) || rate < 8000 || rate > 384000)
    throw std::invalid_argument("Unsupported modal edit sample rate");
  ValidateEnvelope(next);
  auto &patch = Instrument(next);
  const auto recipe = ParseRecipe(patch.at("recipe"));
  auto session = std::make_unique<detail::Session>();
  detail::Initialize(*session, recipe, rate);
  ApplyPatch(*session, patch);
  auto result = std::make_unique<PreparedModalEdit>();
  result->sampleRate = rate;
  result->recipe = recipe;
  using namespace tfdsp::percussion;
  const auto copy = [&](const auto &values) {
    static_assert(std::tuple_size_v<std::decay_t<decltype(values)>> <=
                  CrashMacroCount);
    std::copy(values.begin(), values.end(), result->values.begin());
  };
  if (recipe == detail::Recipe::MetallicPlate) {
    copy(session->crashValues);
    result->metallic = std::make_unique<PreparedMetallicEdit>();
    auto &m = *result->metallic;
    m.parameters = DefaultCrashCymbalParameters(
        rate, ApplyCrashMacros(session->crashBase, result->values));
    m.parameters.routing = session->cymbalRouting;
    m.field.Prepare(rate, m.parameters.modalField,
                    m.parameters.modalFieldControls, 700.f, 6500.f);
  } else {
    MembraneDrumParameters p;
    if (recipe == detail::Recipe::Kick) {
      copy(session->kickValues);
      p = ApplyKickParameters(session->kickValues);
    } else if (recipe == detail::Recipe::SnareDrum) {
      copy(session->snareValues);
      const auto snare = ApplySnareParameters(session->snareValues);
      p = snare.membrane;
      result->wires = PrepareWireRackParameters(rate, snare.wires);
    } else {
      copy(session->membraneValues);
      p = ApplyMembraneParameters(session->membraneValues);
    }
    result->membrane = MembraneResonator<MembraneModeCount>::PrepareParameters(
        rate, p.membrane);
  }
  return result;
}

bool Voice::CanApplyModalEdit() const noexcept {
  if (session_->recipe == detail::Recipe::SnareDrum)
    return session_->snare.CanAdoptModalEdit();
  return session_->recipe != detail::Recipe::MetallicPlate ||
         session_->cymbal.CanAdoptModalEdit();
}

bool Voice::ApplyModalEdit(PreparedModalEdit &edit) noexcept {
  if (session_->recipe != edit.recipe || edit.sampleRate != sampleRate_ ||
      !CanApplyModalEdit())
    return false;
  // Prepared snapshots must not roll back newer sample-timed automation.
  const auto copy = [&](auto &values) {
    for (std::size_t i = 0; i < values.size(); ++i)
      if (!liveIndices_[i])
        values[i] = edit.values[i];
  };
  switch (edit.recipe) {
  case detail::Recipe::MetallicPlate:
    if (!edit.metallic ||
        !session_->cymbal.AdoptModalEdit(edit.metallic->parameters,
                                       edit.metallic->field))
      return false;
    copy(session_->crashValues);
    break;
  case detail::Recipe::Kick:
    copy(session_->kickValues);
    session_->kick.AdoptModalEdit(edit.membrane);
    break;
  case detail::Recipe::MembraneDrum:
    copy(session_->membraneValues);
    session_->membrane.AdoptModalEdit(edit.membrane);
    break;
  case detail::Recipe::SnareDrum:
    if (!session_->snare.AdoptModalEdit(edit.membrane, edit.wires))
      return false;
    copy(session_->snareValues);
    break;
  default:
    return false;
  }
  liveDirty_ = decayDirty_ = true;
  FlushParameters();
  return true;
}
} // namespace drumfoundry
