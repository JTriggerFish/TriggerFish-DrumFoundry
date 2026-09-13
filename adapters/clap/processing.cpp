#include "plugin.hpp"
#include <algorithm>
#include <cmath>

namespace drumfoundry::clap_adapter {
namespace {
bool ValidEvents(const clap_input_events_t *events, uint32_t frames) noexcept {
  if (!events)
    return true;
  if (!events->size || !events->get)
    return false;
  uint32_t previous = 0;
  for (uint32_t i = 0; i < events->size(events); ++i) {
    const auto *e = events->get(events, i);
    if (!e || e->size < sizeof(clap_event_header_t) || e->time < previous ||
        e->time >= frames)
      return false;
    if (e->space_id == CLAP_CORE_EVENT_SPACE_ID &&
        ((e->type == CLAP_EVENT_MIDI && e->size < sizeof(clap_event_midi_t)) ||
         (e->type == CLAP_EVENT_PARAM_VALUE &&
          e->size < sizeof(clap_event_param_value_t))))
      return false;
    previous = e->time;
  }
  return true;
}
} // namespace
void Plugin::Event(const clap_event_header_t *header) noexcept {
  if (!header || header->space_id != CLAP_CORE_EVENT_SPACE_ID)
    return;
  if (header->type == CLAP_EVENT_PARAM_VALUE &&
      header->size >= sizeof(clap_event_param_value_t)) {
    const auto &e = *reinterpret_cast<const clap_event_param_value_t *>(header);
    if (e.note_id == -1 && e.port_index == -1 && e.channel == -1 && e.key == -1)
      SetParameter(e.param_id, e.value);
  } else if (header->type == CLAP_EVENT_MIDI &&
             header->size >= sizeof(clap_event_midi_t) && voice_) {
    const auto &e = *reinterpret_cast<const clap_event_midi_t *>(header);
    if (e.port_index != 0 || e.data[1] >= 128 || e.data[2] >= 128)
      return;
    const auto kind = e.data[0] & 0xf0;
    if (kind == 0x90 && e.data[2]) {
#ifdef DRUMFOUNDRY_UI
      audition_
          .Stop(); // Live strikes interrupt one-shot reference/render audition.
#endif
      auto strike = voice_->Event();
      strike.strength = e.data[2] / 127.f;
      strike.hardness = static_cast<float>(audioValues_[Hardness - Preset]);
      strike.implement = static_cast<float>(audioValues_[Implement - Preset]);
      strike.location = static_cast<float>(audioValues_[Location - Preset]);
      strike.constraint = static_cast<float>(audioValues_[Mute - Preset]);
      voice_->Trigger(strike);
    } else if (kind == 0xb0 && (e.data[1] == 120 || e.data[1] == 123))
      Reset();
  }
}
void Plugin::Render(float *left, float *right, uint32_t frames) noexcept {
  voice_->Process(left, frames);
  for (uint32_t i = 0; i < frames; ++i) {
    masterGain_ += masterStep_ * (masterTarget_ - masterGain_);
    float sample = left[i];
#ifdef DRUMFOUNDRY_UI
    if (audition_.Active())
      sample = audition_.Next();
#endif
    const float source = static_cast<float>(sample * masterGain_);
    const auto output = limiter_.ProcessFrame({source, source});
    left[i] = output[0];
    right[i] = output[1];
  }
}
clap_process_status Plugin::Process(const clap_process_t *p) noexcept {
  if (!active || !processing || !voice_ || !p ||
      p->frames_count > maximumFrames_ || p->audio_outputs_count != 1 ||
      !p->audio_outputs || p->audio_inputs_count != 0)
    return CLAP_PROCESS_ERROR;
  auto &bus = p->audio_outputs[0];
  if (bus.channel_count != 2 || !bus.data32 || !bus.data32[0] || !bus.data32[1])
    return CLAP_PROCESS_ERROR;
  if (!ValidEvents(p->in_events, p->frames_count)) {
    std::fill_n(bus.data32[0], p->frames_count, 0);
    std::fill_n(bus.data32[1], p->frames_count, 0);
    return CLAP_PROCESS_ERROR;
  }
  limiter_.ClearMeters();
#ifdef DRUMFOUNDRY_UI
  if (audition_.Begin(unsigned(std::lround(sampleRate_))))
    voice_->Reset();
#endif
  DrainEditor(p->out_events, true);
  uint32_t cursor = 0;
  const auto count = p->in_events ? p->in_events->size(p->in_events) : 0;
  for (uint32_t i = 0; i < count; ++i) {
    const auto *e = p->in_events->get(p->in_events, i);
    Render(bus.data32[0] + cursor, bus.data32[1] + cursor, e->time - cursor);
    Event(e);
    cursor = e->time;
  }
  Render(bus.data32[0] + cursor, bus.data32[1] + cursor,
         p->frames_count - cursor);
  bus.constant_mask = 0;
  reductionHold_ = std::max(
      limiter_.Status().maximumReductionDb,
      reductionHold_ * std::exp(-double(p->frames_count) / (.3 * sampleRate_)));
  values_[Reduction - Preset].store(reductionHold_);
  return CLAP_PROCESS_CONTINUE;
}
} // namespace drumfoundry::clap_adapter
