#include "plugin.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace drumfoundry::clap_adapter {
namespace {
bool InRange(clap_id id, double value) noexcept {
  if (const auto *p = FindDesignParameter(id))
    return ValidDesignValue(*p, value);
  return id >= Preset && id < ParameterEnd && std::isfinite(value) &&
         value >= Controls[id - Preset].low &&
         value <= Controls[id - Preset].high;
}
} // namespace
const std::array<Control, ParameterCount> Controls{{
    {"Factory preset", "Preset", 0, 5, 2, true, false},
    {"Hardness", "Strike", 0, 1, .5, false, false},
    {"Implement (brush–mallet–stick)", "Strike", 0, 1, .5, false, false},
    {"Location (fixed for kick)", "Strike", 0, 1, 0, false, false},
    {"Mute (metallic voices)", "Strike", 0, 1, 0, false, false},
    {"Master level", "Output", -60, 0, -12, false, false},
    {"Limiter", "Output", 0, 1, 1, true, false},
    {"Gain reduction", "Output", 0, 1000, 0, false, true},
    {"Latency (ms)", "Output", 0, 2, 0, false, true},
    {"Gesture spread", "Strike", 0, 1, .2, false, false},
}};
bool ValidValue(clap_id id, double value) noexcept {
  if (const auto *p = FindDesignParameter(id))
    return ValidDesignValue(*p, value);
  if (id < Preset || id >= ParameterEnd || !std::isfinite(value))
    return false;
  const auto &c = Controls[id - Preset];
  return !c.readonly && value >= c.low && value <= c.high &&
         (!c.stepped || value == std::floor(value));
}
double Plugin::Value(clap_id id) const noexcept {
  if (const auto slot = DesignSlot(id); slot < DesignCapacity)
    return values_[ParameterCount + slot].load();
  return id >= Preset && id < ParameterEnd ? values_[id - Preset].load() : 0;
}
void Plugin::SetParameter(clap_id id, double value) noexcept {
  if (FindDesignParameter(id)) {
    if (Value(id) != value)
      SetDesignParameter(id, value);
    return;
  }
  if (!ValidValue(id, value) || Value(id) == value)
    return;
  values_[id - Preset].store(value);
  if (id == Preset || id == Protection) {
    if (active)
      RequestRestart();
    return;
  }
  audioValues_[id - Preset] = value;
  if (id == Master)
    masterTarget_ = std::pow(10., value / 20);
  if (id == Mute && voice_)
    voice_->SetMute(static_cast<float>(value));
}
const clap_plugin_params_t ParamsExtension{
    [](const clap_plugin_t *) -> uint32_t {
      return ParameterCount + DesignParameters().size();
    },
    [](const clap_plugin_t *plugin, uint32_t index, clap_param_info_t *info) {
      if (!info || index >= ParameterCount + DesignParameters().size())
        return false;
      if (index >= ParameterCount) {
        const auto &p = DesignParameters()[index - ParameterCount];
        const auto &d = *p.descriptor;
        *info = {};
        info->id = p.id;
        info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_REQUIRES_PROCESS;
        if (int(d.scale) >= 2)
          info->flags |= CLAP_PARAM_IS_STEPPED;
        if (p.recipe != Plugin::Get(plugin).DesignRecipe())
          info->flags |= CLAP_PARAM_IS_HIDDEN;
        std::snprintf(info->name, sizeof(info->name), "%s", d.name.c_str());
        std::snprintf(info->module, sizeof(info->module), "%s",
                      p.module.c_str());
        info->min_value = d.minimum;
        info->max_value = d.maximum;
        info->default_value = d.defaultValue;
        return true;
      }
      const auto &c = Controls[index];
      *info = {};
      info->id = Preset + index;
      if (c.readonly)
        info->flags = CLAP_PARAM_IS_READONLY;
      else if (info->id != Preset && info->id != Protection)
        info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_REQUIRES_PROCESS;
      if (c.stepped)
        info->flags |= CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM;
      std::snprintf(info->name, sizeof(info->name), "%s", c.name);
      std::snprintf(info->module, sizeof(info->module), "%s", c.group);
      info->min_value = c.low;
      info->max_value = c.high;
      info->default_value = c.initial;
      return true;
    },
    [](const clap_plugin_t *p, clap_id id, double *value) {
      if (!value ||
          ((id < Preset || id >= ParameterEnd) && !FindDesignParameter(id)))
        return false;
      *value = FindDesignParameter(id) ? Plugin::Get(p).EditorValue(id)
                                       : Plugin::Get(p).Value(id);
      return true;
    },
    [](const clap_plugin_t *, clap_id id, double value, char *text,
       uint32_t size) {
      if (!text || !size || !InRange(id, value))
        return false;
      if (const auto *p = FindDesignParameter(id)) {
        const auto &d = *p->descriptor;
        if (d.scale == ParameterScale::Boolean)
          std::snprintf(text, size, "%s", value >= .5 ? "On" : "Off");
        else
          std::snprintf(text, size, "%.6g%s%s", value,
                        d.unit.empty() ? "" : " ", d.unit.c_str());
        return true;
      }
      if (id == Preset) {
        std::snprintf(text, size, "%s",
                      PresetNames[static_cast<std::size_t>(std::round(value))]);
      } else if (id == Protection) {
        std::snprintf(text, size, "%s",
                      value >= .5 ? "On — 1 ms" : "Off — unprotected");
      } else
        std::snprintf(text, size, "%.3f%s", value,
                      id == Master || id == Reduction ? " dB"
                      : id == Latency                 ? " ms"
                                                      : "");
      return true;
    },
    [](const clap_plugin_t *, clap_id id, const char *text, double *value) {
      if (!text || !value)
        return false;
      if (const auto *p = FindDesignParameter(id)) {
        if (p->descriptor->scale == ParameterScale::Boolean) {
          if (!std::strcmp(text, "On")) {
            *value = 1;
            return true;
          }
          if (!std::strcmp(text, "Off")) {
            *value = 0;
            return true;
          }
        }
        char *end = nullptr;
        const double parsed = std::strtod(text, &end);
        const auto suffix = std::string(" ") + p->descriptor->unit;
        if (end == text || (*end && suffix != end) ||
            !ValidDesignValue(*p, parsed))
          return false;
        *value = parsed;
        return true;
      }
      if (id == Preset)
        for (std::size_t i = 0; i < PresetNames.size(); ++i)
          if (!std::strcmp(text, PresetNames[i])) {
            *value = double(i);
            return true;
          }
      if (id == Protection) {
        if (!std::strcmp(text, "On — 1 ms")) {
          *value = 1;
          return true;
        }
        if (!std::strcmp(text, "Off — unprotected")) {
          *value = 0;
          return true;
        }
      }
      char *end = nullptr;
      const double parsed = std::strtod(text, &end);
      if (end == text ||
          (*end && std::strcmp(end, " dB") && std::strcmp(end, " ms")) ||
          !InRange(id, parsed))
        return false;
      *value = parsed;
      return true;
    },
    [](const clap_plugin_t *p, const clap_input_events_t *in,
       const clap_output_events_t *out) {
      Plugin::Get(p).DrainEditor(out, false);
      if (!in || !in->size || !in->get)
        return;
      Plugin::Get(p).EventBatch(in, 0, in->size(in), false);
    }};
} // namespace drumfoundry::clap_adapter
