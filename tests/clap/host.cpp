#include "host.hpp"
#include "drumfoundry/version.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace clap_test {
void Require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
namespace {
clap_input_events_t
Events(const std::vector<const clap_event_header_t *> &events) {
  return {
      const_cast<void *>(static_cast<const void *>(&events)),
      [](const clap_input_events_t *in) -> uint32_t {
        return static_cast<uint32_t>(
            static_cast<const std::vector<const clap_event_header_t *> *>(
                in->ctx)
                ->size());
      },
      [](const clap_input_events_t *in, uint32_t index) {
        return static_cast<const std::vector<const clap_event_header_t *> *>(
                   in->ctx)
            ->at(index);
      }};
}
} // namespace
Host::Host(const char *path) {
#ifdef _WIN32
  library_ = LoadLibraryA(path);
  if (library_)
    entry_ = reinterpret_cast<const clap_plugin_entry_t *>(
        GetProcAddress(static_cast<HMODULE>(library_), "clap_entry"));
#else
  library_ = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (library_)
    entry_ =
        static_cast<const clap_plugin_entry_t *>(dlsym(library_, "clap_entry"));
#endif
  Require(entry_ && entry_->init(path), "load CLAP entry");
  host_ = {CLAP_VERSION, this, "DrumFoundry test host", "TriggerFish", "", "1"};
  host_.request_restart = [](const clap_host_t *h) {
    ++static_cast<Host *>(h->host_data)->restarts;
  };
  host_.request_process = [](const clap_host_t *) {};
  host_.request_callback = [](const clap_host_t *) {};
  host_.get_extension = [](const clap_host_t *,
                           const char *id) -> const void * {
    static const clap_host_latency_t latency{[](const clap_host_t *h) {
      ++static_cast<Host *>(h->host_data)->latencyChanges;
    }};
    static const clap_host_params_t params{
        [](const clap_host_t *h, clap_param_rescan_flags) {
          ++static_cast<Host *>(h->host_data)->rescans;
        },
        [](const clap_host_t *, clap_id, clap_param_clear_flags) {},
        [](const clap_host_t *) {}};
    if (!std::strcmp(id, CLAP_EXT_LATENCY))
      return &latency;
    if (!std::strcmp(id, CLAP_EXT_PARAMS))
      return &params;
    return nullptr;
  };
  auto *factory = static_cast<const clap_plugin_factory_t *>(
      entry_->get_factory(CLAP_PLUGIN_FACTORY_ID));
  Require(factory && factory->get_plugin_count(factory) == 1,
          "one instrument factory");
  Require(!factory->get_plugin_descriptor(factory, 1), "invalid factory index");
  const auto *descriptor = factory->get_plugin_descriptor(factory, 0);
  Require(descriptor && descriptor->version &&
              std::strcmp(descriptor->version, drumfoundry::Version) == 0,
          "CLAP product version matches native build");
  plugin = factory->create_plugin(factory, &host_, descriptor->id);
  Require(plugin && plugin->init(plugin), "initialize plugin");
  params = static_cast<const clap_plugin_params_t *>(
      plugin->get_extension(plugin, CLAP_EXT_PARAMS));
  latency = static_cast<const clap_plugin_latency_t *>(
      plugin->get_extension(plugin, CLAP_EXT_LATENCY));
  state = static_cast<const clap_plugin_state_t *>(
      plugin->get_extension(plugin, CLAP_EXT_STATE));
  Require(params && latency && state, "required extensions");
}
Host::~Host() {
  Stop();
  if (plugin)
    plugin->destroy(plugin);
  if (entry_)
    entry_->deinit();
#ifdef _WIN32
  if (library_)
    FreeLibrary(static_cast<HMODULE>(library_));
#else
  if (library_)
    dlclose(library_);
#endif
}
void Host::Start(double rate) {
  Require(!started_ && plugin->activate(plugin, rate, 1, 512), "activate");
  Require(plugin->start_processing(plugin), "start processing");
  started_ = true;
}
void Host::Stop() {
  if (!started_)
    return;
  plugin->stop_processing(plugin);
  plugin->deactivate(plugin);
  started_ = false;
}
void Host::Set(clap_id id, double value) {
  clap_event_param_value_t event{};
  event.header = {sizeof(event), 0, CLAP_CORE_EVENT_SPACE_ID,
                  CLAP_EVENT_PARAM_VALUE, 0};
  event.param_id = id;
  event.note_id = event.port_index = event.channel = event.key = -1;
  event.value = value;
  const std::vector<const clap_event_header_t *> list{&event.header};
  const auto in = Events(list);
  params->flush(plugin, &in, nullptr);
}
double Host::Get(clap_id id) const {
  double value = 0;
  Require(params->get_value(plugin, id, &value), "get parameter");
  return value;
}
clap_process_status
Host::Render(uint32_t frames,
             const std::vector<const clap_event_header_t *> &events) {
  Require(frames <= left.size(), "test buffer bounds");
  float *channels[]{left.data(), right.data()};
  clap_audio_buffer_t bus{channels, nullptr, 2, 0, 0};
  const auto in = Events(events);
  clap_process_t process{};
  process.frames_count = frames;
  process.audio_outputs = &bus;
  process.audio_outputs_count = 1;
  process.in_events = &in;
  return plugin->process(plugin, &process);
}
clap_event_midi_t Note(uint32_t offset, uint8_t velocity) {
  clap_event_midi_t e{};
  e.header = {sizeof(e), offset, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_MIDI, 0};
  e.data[0] = 0x90;
  e.data[1] = 60;
  e.data[2] = velocity;
  return e;
}
} // namespace clap_test
