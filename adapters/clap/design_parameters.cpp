#include "design_parameters.hpp"
#include "parameters/access.hpp"
#include <cmath>
#include <stdexcept>

namespace drumfoundry::clap_adapter {
namespace {
clap_id StableId(const std::string &recipe, const std::string &key) {
  // FNV-1a, restricted to a separate positive 30-bit host-ID namespace.
  std::uint32_t hash = 2166136261u;
  for (unsigned char c : recipe + "/" + key)
    hash = (hash ^ c) * 16777619u;
  return 0x40000000u | (hash & 0x3fffffffu);
}
} // namespace
const std::vector<DesignParameter> &DesignParameters() {
  static const auto table = [] {
    std::vector<DesignParameter> result;
    constexpr const char *labels[]{"Metal", "Kick", "Membrane", "Snare"};
    for (unsigned recipe = 0; recipe < unsigned(detail::Recipe::Count);
         ++recipe) {
      const auto kind = detail::Recipe(recipe);
      const auto name = Topology(kind).at("recipe").get<std::string>();
      for (std::size_t i = 0; i < detail::ParameterCount(kind); ++i) {
        const auto *d = detail::Description(kind, i);
        if (!IsLiveParameter(name, d->key))
          continue;
        const auto id = StableId(name, d->key);
        for (const auto &p : result)
          if (p.id == id)
            throw std::logic_error("CLAP design parameter ID collision");
        const std::string owner(ParameterOwner(kind, d->key));
        result.push_back(
            {id, kind, i, d, owner, std::string(labels[recipe]) + "/" + owner});
      }
    }
    if (result.size() > DesignCapacity)
      throw std::logic_error("CLAP design capacity exceeded");
    return result;
  }();
  return table;
}
std::size_t DesignSlot(clap_id id) noexcept {
  const auto &table = DesignParameters();
  for (std::size_t i = 0; i < table.size(); ++i)
    if (table[i].id == id)
      return i;
  return DesignCapacity;
}
const DesignParameter *FindDesignParameter(clap_id id) noexcept {
  const auto slot = DesignSlot(id);
  return slot < DesignCapacity ? &DesignParameters()[slot] : nullptr;
}
bool ValidDesignValue(const DesignParameter &p, double value) noexcept {
  const auto &d = *p.descriptor;
  return std::isfinite(value) && float(value) >= d.minimum &&
         float(value) <= d.maximum &&
         (int(d.scale) < 2 || value == std::floor(value));
}
int DesignEditPriority(const DesignParameter &p, double value,
                       double current) noexcept {
  const auto &key = p.descriptor->key;
  if (key.rfind("body_decay_active_", 0) == 0)
    return value < .5 ? 0 : 3;
  if (key == "body_decay_frequency_7")
    return value > current ? 1 : 4;
  return 2;
}
} // namespace drumfoundry::clap_adapter
