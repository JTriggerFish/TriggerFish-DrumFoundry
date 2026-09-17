#include "plugin_host.hpp"
#include "drumfoundry/version.hpp"
#include <cstring>
#include <sstream>
#include <stdexcept>
extern "C" const clap_plugin_entry_t clap_entry;

namespace drumfoundry::standalone {
PluginHost::PluginHost() {
  if (!clap_entry.init(""))
    throw std::runtime_error("CLAP entry initialization failed");
  host_ = {CLAP_VERSION,  this, "DrumFoundry standalone",
           "TriggerFish", "",   Version};
  host_.request_restart = [](const clap_host_t *h) {
    static_cast<PluginHost *>(h->host_data)->restart = true;
  };
  host_.request_callback = [](const clap_host_t *h) {
    static_cast<PluginHost *>(h->host_data)->callbackRequested_ = true;
  };
  host_.request_process = [](const clap_host_t *) {};
  host_.get_extension = [](const clap_host_t *,
                           const char *id) -> const void * {
    static const clap_host_latency_t latency{[](const clap_host_t *) {}};
    static const clap_host_params_t params{
        [](const clap_host_t *, clap_param_rescan_flags) {},
        [](const clap_host_t *, clap_id, clap_param_clear_flags) {},
        [](const clap_host_t *h) {
          static_cast<PluginHost *>(h->host_data)->flushRequested_ = true;
        }};
    if (!std::strcmp(id, CLAP_EXT_LATENCY))
      return &latency;
    if (!std::strcmp(id, CLAP_EXT_PARAMS))
      return &params;
    return nullptr;
  };
  const auto *factory = static_cast<const clap_plugin_factory_t *>(
      clap_entry.get_factory(CLAP_PLUGIN_FACTORY_ID));
  if (factory) {
    const auto *descriptor = factory->get_plugin_descriptor(factory, 0);
    if (descriptor)
      plugin_ = factory->create_plugin(factory, &host_, descriptor->id);
  }
  if (!plugin_ || !plugin_->init(plugin_)) {
    if (plugin_)
      plugin_->destroy(plugin_);
    clap_entry.deinit();
    throw std::runtime_error("Cannot initialize DrumFoundry CLAP");
  }
  params_ = static_cast<const clap_plugin_params_t *>(
      plugin_->get_extension(plugin_, CLAP_EXT_PARAMS));
  if (!params_) {
    plugin_->destroy(plugin_);
    clap_entry.deinit();
    throw std::runtime_error("DrumFoundry parameter extension missing");
  }
}
PluginHost::~PluginHost() {
  Stop();
  plugin_->destroy(plugin_);
  clap_entry.deinit();
}
void PluginHost::Prepare(double rate, unsigned maximumFrames) {
  Stop();
  if (!maximumFrames || maximumFrames > 16384)
    throw std::runtime_error("Unsupported device block size");
  left_.assign(maximumFrames, 0);
  right_.assign(maximumFrames, 0);
  if (!plugin_->activate(plugin_, rate, 1, maximumFrames))
    throw std::runtime_error("Plugin activation failed");
  maximum_ = maximumFrames;
  active_ = true;
  restart = false;
}
void PluginHost::Stop() noexcept {
  if (processing_)
    plugin_->stop_processing(plugin_);
  if (active_)
    plugin_->deactivate(plugin_);
  processing_ = active_ = false;
  // Main-thread controls have no producer here. Preserve edits, but never
  // replay queued strikes into a newly selected preset/device configuration.
  Event event;
  while (controls.Pop(event))
    if (!event.midi)
      SetStopped(event.parameter, event.value);
  for (auto &queue : midi)
    queue.DiscardPending();
}
void PluginHost::SetStopped(clap_id id, double value) {
  if (active_)
    throw std::logic_error("Stop audio before structural changes");
  count_ = 0;
  Add({false, id, value, {}});
  const clap_input_events_t events{
      this, [](const clap_input_events_t *) -> uint32_t { return 1; },
      [](const clap_input_events_t *in, uint32_t) {
        return static_cast<PluginHost *>(in->ctx)->events_[0];
      }};
  params_->flush(plugin_, &events, nullptr);
}
double PluginHost::Value(clap_id id) const {
  double value = 0;
  if (!params_->get_value(plugin_, id, &value))
    throw std::runtime_error("Unknown host control");
  return value;
}
void PluginHost::Service() {
  // With no device callback, honor CLAP's inactive main-thread flush contract
  // so editing cannot accumulate an unbounded backlog waiting for playback.
  if (!active_ && flushRequested_.exchange(false))
    params_->flush(plugin_, nullptr, nullptr);
  if (callbackRequested_.exchange(false))
    plugin_->on_main_thread(plugin_);
}
unsigned PluginHost::DroppedEvents() const noexcept {
  unsigned dropped = controls.Dropped();
  for (const auto &queue : midi)
    dropped += queue.Dropped();
  return dropped;
}
std::string PluginHost::Status() const {
  std::ostringstream out;
  out << "Limiter " << (Value(106) ? "ON" : "OFF — UNPROTECTED")
      << " | lookahead " << Value(108) << " ms | reduction " << Value(107)
      << " dB | master " << Value(105) << " dB | MIDI " << midiReceived.load()
      << " | dropped events " << DroppedEvents() << " | process errors "
      << failures.load();
  return out.str();
}
} // namespace drumfoundry::standalone
