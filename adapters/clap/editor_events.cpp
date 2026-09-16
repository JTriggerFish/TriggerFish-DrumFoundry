#include "plugin.hpp"
#include <algorithm>
#include <cmath>

namespace drumfoundry::clap_adapter {
namespace {
std::size_t Slot(clap_id id) noexcept {
  return id >= Preset && id < ParameterEnd ? id - Preset
                                           : ParameterCount + DesignSlot(id);
}
} // namespace
double Plugin::EditorValue(clap_id id) const noexcept {
  const auto index = Slot(id);
  if (index >= AllParameterSlots)
    return 0;
  if (index >= ParameterCount && designPending_)
    return desiredDesignValues_[index - ParameterCount];
  const auto pending = pendingEdits_[index];
  return pending.serial >
                 acknowledgedEdits_[index].load(std::memory_order_acquire)
             ? pending.value
             : Value(id);
}
void Plugin::CancelEditorEdits(bool includeMonitor) {
  for (std::size_t index = 0; index < AllParameterSlots; ++index) {
    if (!includeMonitor &&
        (index == Master - Preset || index == Protection - Preset))
      continue;
    cancelledEdits_[index].store(editSerial_, std::memory_order_release);
    pendingEdits_[index] = {};
  }
}
bool Plugin::QueueEdit(clap_id id, double value) noexcept {
  const auto serial = editSerial_ + 1;
  const auto design = DesignSlot(id);
  if (design < DesignCapacity &&
      (designPending_ ||
       DesignParameters()[design].recipe != designRecipe_.load()))
    return false;
  const unsigned gesture =
      design < DesignCapacity ? (designGesturesMain_[design] ? 2 : 1) : 0;
  if (!ValidValue(id, value) || editorParams_->Available() <= DesignCapacity ||
      !editorParams_->Push({false, id, value, {}, 0, serial, gesture}))
    return false;
  if (design < DesignCapacity)
    designGesturesMain_[design] = true;
  editSerial_ = serial;
  pendingEdits_[Slot(id)] = {value, serial};
  if (hostParams_ && hostParams_->request_flush)
    hostParams_->request_flush(host_);
  if (host_->request_process)
    host_->request_process(host_);
  return true;
}
bool Plugin::QueueStrike(float velocity, float location) noexcept {
  if (!std::isfinite(velocity) || !std::isfinite(location) || velocity <= 0 ||
      velocity > 1 || location < 0 || location > 1)
    return false;
  if (!editorNotes_.Push({true, 0, location, {0x90, 60, 1}, velocity}))
    return false;
  if (host_->request_process)
    host_->request_process(host_);
  return true;
}
bool Plugin::QueuePanic() noexcept {
  if (!editorNotes_.Push({true, 0, 0, {0xb0, 120, 0}}))
    return false;
  if (host_->request_process)
    host_->request_process(host_);
  return true;
}
void Plugin::DrainEditor(const clap_output_events_t *out, bool notes) noexcept {
  DesignWrite publication(*this);
  host::Event event;
  // Bounded even if the UI producer is active throughout this callback.
  for (unsigned n = 0; n < 2047 && editorParams_->Pop(event); ++n) {
    const auto slot = Slot(event.parameter);
    if (slot >= AllParameterSlots)
      continue;
    const auto design = DesignSlot(event.parameter);
    if (event.gesture == 3) {
      if (design < DesignCapacity && designGesturesAudio_[design]) {
        clap_event_param_gesture_t end{{sizeof(end), 0,
                                        CLAP_CORE_EVENT_SPACE_ID,
                                        CLAP_EVENT_PARAM_GESTURE_END, 0},
                                       event.parameter};
        if (out && out->try_push && !out->try_push(out, &end.header))
          ++editorNotificationErrors_;
        designGesturesAudio_[design] = false;
      }
      continue;
    }
    if (event.editSerial <=
        cancelledEdits_[slot].load(std::memory_order_acquire))
      continue;
    SetParameter(event.parameter, event.value);
    acknowledgedEdits_[slot].store(event.editSerial, std::memory_order_release);
    if (design < DesignCapacity)
      ++automationRevision_;
    const bool begin = !event.gesture || !designGesturesAudio_[design];
    if (!out || !out->try_push)
      continue;
    clap_event_param_gesture_t gesture{{sizeof(gesture), 0,
                                        CLAP_CORE_EVENT_SPACE_ID,
                                        CLAP_EVENT_PARAM_GESTURE_BEGIN, 0},
                                       event.parameter};
    clap_event_param_value_t value{};
    value.header = {sizeof(value), 0, CLAP_CORE_EVENT_SPACE_ID,
                    CLAP_EVENT_PARAM_VALUE, 0};
    value.param_id = event.parameter;
    value.value =
        Value(event.parameter); // Report the accepted, validated value.
    value.note_id = value.port_index = value.channel = value.key = -1;
    bool sent = !begin || out->try_push(out, &gesture.header);
    if (event.gesture && sent)
      designGesturesAudio_[design] = true;
    sent = out->try_push(out, &value.header) && sent;
    gesture.header.type = CLAP_EVENT_PARAM_GESTURE_END;
    if (!event.gesture)
      sent = out->try_push(out, &gesture.header) && sent;
    if (!sent)
      ++editorNotificationErrors_;
  }
  if (!notes)
    return; // Flush must not swallow manual strikes before activation.
  for (unsigned n = 0; n < 255 && editorNotes_.Pop(event); ++n) {
    if ((event.bytes[0] & 0xf0) == 0x90)
      SetParameter(fixedBeater_ ? Hardness : Location, event.value);
    if (event.strikeVelocity > 0) {
      StrikeVoice(event.strikeVelocity);
      continue;
    }
    clap_event_midi_t midi{};
    midi.header = {sizeof(midi), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI,
                   0};
    std::copy(event.bytes.begin(), event.bytes.end(), midi.data);
    Event(&midi.header);
  }
}
} // namespace drumfoundry::clap_adapter
