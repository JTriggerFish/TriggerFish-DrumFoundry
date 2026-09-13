#include "plugin.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace drumfoundry::clap_adapter {
namespace {
bool InRange(clap_id id, double value) noexcept {
  return id >= Preset && id < ParameterEnd && std::isfinite(value) &&
         value >= Controls[id - Preset].low &&
         value <= Controls[id - Preset].high;
}
} // namespace
const std::array<Control, ParameterCount> Controls{{
    {"Instrument", "Preset", 0, 5, 0, true, false},
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
  if (id < Preset || id >= ParameterEnd || !std::isfinite(value))
    return false;
  const auto &c = Controls[id - Preset];
  return !c.readonly && value >= c.low && value <= c.high &&
         (!c.stepped || value == std::floor(value));
}
double Plugin::Value(clap_id id) const noexcept {
  return id >= Preset && id < ParameterEnd ? values_[id - Preset].load() : 0;
}
void Plugin::SetParameter(clap_id id, double value) noexcept {
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
    [](const clap_plugin_t *) -> uint32_t { return ParameterCount; },
    [](const clap_plugin_t *, uint32_t index, clap_param_info_t *info) {
      if (!info || index >= Controls.size())
        return false;
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
      if (!value || id < Preset || id >= ParameterEnd)
        return false;
      *value = Plugin::Get(p).Value(id);
      return true;
    },
    [](const clap_plugin_t *, clap_id id, double value, char *text,
       uint32_t size) {
      if (!text || !size || !InRange(id, value))
        return false;
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
      for (uint32_t i = 0; i < in->size(in); ++i) {
        const auto *e = in->get(in, i);
        if (e && e->size >= sizeof(clap_event_header_t) &&
            e->type == CLAP_EVENT_PARAM_VALUE)
          Plugin::Get(p).Event(e);
      }
    }};
} // namespace drumfoundry::clap_adapter
