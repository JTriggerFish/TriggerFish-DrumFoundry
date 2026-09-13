#pragma once
#include "event_queue.hpp"
#include <string>
#include <vector>

namespace drumfoundry::standalone {
// Main thread prepares/stops; device callback alone processes while running.
// The same CLAP entry/object code is linked into both application and plugin.
class PluginHost {
public:
  PluginHost();
  ~PluginHost();
  PluginHost(const PluginHost &) = delete;
  PluginHost &operator=(const PluginHost &) = delete;
  void Prepare(double rate, unsigned maximumFrames);
  void Stop() noexcept; // Caller must join/stop audio callbacks first.
  void Process(float *interleaved, unsigned frames) noexcept;
  void SetStopped(clap_id id, double value);
  double Value(clap_id id) const;
  void Service();
  std::string Status() const;
  const clap_plugin_t *Api() const { return plugin_; }
  unsigned DroppedEvents() const noexcept;
  EventQueue<> controls;
  std::array<EventQueue<>, 32> midi;
  std::atomic<unsigned> failures{}, callbacks{}, midiReceived{};
  std::atomic<bool> restart{};

private:
  void Collect() noexcept;
  void Add(Event e) noexcept;
  const clap_plugin_t *plugin_{};
  const clap_plugin_params_t *params_{};
  clap_host_t host_{};
  std::atomic<bool> callbackRequested_{};
  bool active_{}, processing_{};
  unsigned maximum_{}, count_{};
  std::size_t nextQueue_{};
  std::vector<float> left_, right_;
  std::array<clap_event_midi_t, 256> midiEvents_{};
  std::array<clap_event_param_value_t, 256> paramEvents_{};
  std::array<const clap_event_header_t *, 256> events_{};
};
} // namespace drumfoundry::standalone
