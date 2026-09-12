#include "plugin_host.hpp"
#include <algorithm>

namespace drumfoundry::standalone {
void PluginHost::Add(Event e) noexcept {
  if (count_ >= events_.size())
    return;
  if (e.midi) {
    auto &m = midiEvents_[count_];
    m = {};
    m.header = {sizeof(m), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI, 0};
    std::copy(e.bytes.begin(), e.bytes.end(), m.data);
    events_[count_] = &m.header;
  } else {
    auto &p = paramEvents_[count_];
    p = {};
    p.header = {sizeof(p), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE,
                0};
    p.param_id = e.parameter;
    p.value = e.value;
    p.note_id = p.port_index = p.channel = p.key = -1;
    events_[count_] = &p.header;
  }
  ++count_;
}
void PluginHost::Collect() noexcept {
  // Rotate across the control queue and all ports, retaining FIFO per source.
  // A busy early port must not starve later keyboards or control changes.
  std::size_t empty = 0;
  while (count_ < events_.size() && empty <= midi.size()) {
    auto &queue = nextQueue_ == 0 ? controls : midi[nextQueue_ - 1];
    nextQueue_ = (nextQueue_ + 1) % (midi.size() + 1);
    Event e;
    if (queue.Pop(e)) {
      empty = 0;
      if (e.midi)
        ++midiReceived;
      Add(e);
    } else {
      ++empty;
    }
  }
}
void PluginHost::Process(float *output, unsigned frames) noexcept {
  if (!output)
    return;
  ++callbacks;
  if (!active_ || frames > maximum_) {
    std::fill_n(output, frames * 2, 0);
    ++failures;
    return;
  }
  if (!processing_)
    processing_ = plugin_->start_processing(plugin_);
  count_ = 0;
  Collect();
  const clap_input_events_t in{
      this,
      [](const clap_input_events_t *e) {
        return static_cast<PluginHost *>(e->ctx)->count_;
      },
      [](const clap_input_events_t *e, uint32_t i) {
        return static_cast<PluginHost *>(e->ctx)->events_[i];
      }};
  float *channels[]{left_.data(), right_.data()};
  clap_audio_buffer_t bus{channels, nullptr, 2, 0, 0};
  clap_process_t process{};
  process.frames_count = frames;
  process.audio_outputs = &bus;
  process.audio_outputs_count = 1;
  process.in_events = &in;
  if (!processing_ ||
      plugin_->process(plugin_, &process) == CLAP_PROCESS_ERROR) {
    std::fill_n(output, frames * 2, 0);
    ++failures;
    return;
  }
  for (unsigned i = 0; i < frames; ++i) {
    output[2 * i] = left_[i];
    output[2 * i + 1] = right_[i];
  }
}
} // namespace drumfoundry::standalone
