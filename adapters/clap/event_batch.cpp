#include "plugin.hpp"

namespace drumfoundry::clap_adapter {
namespace {
bool ParameterEvent(const clap_event_header_t *e) {
  return e && e->space_id == CLAP_CORE_EVENT_SPACE_ID &&
         e->type == CLAP_EVENT_PARAM_VALUE &&
         e->size >= sizeof(clap_event_param_value_t);
}
} // namespace

void Plugin::EventBatch(const clap_input_events_t *in, uint32_t begin,
                        uint32_t end, bool notes) noexcept {
  const auto &table = DesignParameters();
  std::array<double, DesignCapacity> next{};
  std::array<bool, DesignCapacity> changed{};
  CrashMacroValues curve{};
  const auto recipe =
      active && voice_ ? voice_->Recipe() : designRecipe_.load();
  for (std::size_t i = 0; i < table.size(); ++i)
    next[i] = values_[ParameterCount + i].load();
  for (auto n = begin; n < end; ++n) {
    const auto *header = in->get(in, n);
    if (!ParameterEvent(header))
      continue;
    const auto &e = *reinterpret_cast<const clap_event_param_value_t *>(header);
    const auto slot = DesignSlot(e.param_id);
    if (slot == DesignCapacity) {
      Event(header);
      continue;
    }
    const auto &p = table[slot];
    if (p.recipe == recipe && ValidDesignValue(p, e.value) && e.note_id == -1 &&
        e.port_index == -1 && e.channel == -1 && e.key == -1) {
      next[slot] = e.value;
      changed[slot] = true;
    }
  }
  for (std::size_t i = 0; i < table.size(); ++i)
    if (table[i].recipe == detail::Recipe::MetallicPlate)
      curve[table[i].index] = float(next[i]);
  const bool validCurve =
      recipe != detail::Recipe::MetallicPlate || ValidLiveDecay(curve);
  {
    DesignWrite publication(*this);
    // Validate the final curve, then order its scalar setters so none sees an
    // invalid intermediate state. Unrelated controls survive a rejected curve.
    for (int priority = 0; priority < 5; ++priority)
      for (std::size_t i = 0; i < table.size(); ++i) {
        const auto &p = table[i];
        if (changed[i] &&
            (validCurve || p.descriptor->key.rfind("body_decay_", 0) != 0) &&
            DesignEditPriority(p, next[i], Value(p.id)) == priority) {
          SetParameter(p.id, next[i]);
          changed[i] = false;
        }
      }
  }
  // Notes at this timestamp observe the completed parameter batch.
  if (notes)
    for (auto n = begin; n < end; ++n)
      if (!ParameterEvent(in->get(in, n)))
        Event(in->get(in, n));
}
} // namespace drumfoundry::clap_adapter
