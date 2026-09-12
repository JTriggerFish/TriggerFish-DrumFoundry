#include "plugin.hpp"
#include <cstdio>

namespace drumfoundry::clap_adapter {
const clap_plugin_audio_ports_t AudioExtension{
    [](const clap_plugin_t *, bool input) -> uint32_t { return input ? 0 : 1; },
    [](const clap_plugin_t *, uint32_t index, bool input,
       clap_audio_port_info_t *info) {
      if (!info || input || index)
        return false;
      *info = {};
      info->id = 0;
      std::snprintf(info->name, sizeof(info->name), "Output");
      info->flags = CLAP_AUDIO_PORT_IS_MAIN;
      info->channel_count = 2;
      info->port_type = CLAP_PORT_STEREO;
      info->in_place_pair = CLAP_INVALID_ID;
      return true;
    }};
const clap_plugin_note_ports_t NoteExtension{
    [](const clap_plugin_t *, bool input) -> uint32_t { return input ? 1 : 0; },
    [](const clap_plugin_t *, uint32_t index, bool input,
       clap_note_port_info_t *info) {
      if (!info || !input || index)
        return false;
      *info = {};
      info->id = 0;
      info->supported_dialects = info->preferred_dialect =
          CLAP_NOTE_DIALECT_MIDI;
      std::snprintf(info->name, sizeof(info->name), "Percussion strikes");
      return true;
    }};
const clap_plugin_latency_t LatencyExtension{
    [](const clap_plugin_t *p) -> uint32_t {
      return Plugin::Get(p).LatencySamples();
    }};
const clap_plugin_state_t StateExtension{
    [](const clap_plugin_t *p, const clap_ostream_t *stream) noexcept {
      try {
        return Plugin::Get(p).Save(stream);
      } catch (...) {
        return false;
      }
    },
    [](const clap_plugin_t *p, const clap_istream_t *stream) noexcept {
      try {
        return Plugin::Get(p).Load(stream);
      } catch (...) {
        return false;
      }
    }};
} // namespace drumfoundry::clap_adapter
