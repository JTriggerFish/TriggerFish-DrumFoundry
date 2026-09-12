#pragma once
#include <array>
#include <clap/clap.h>
#include <string>
#include <vector>

namespace clap_test {
void Require(bool condition, const char *message);

// Owns the dynamically loaded plugin, with no static link to its
// implementation.
class Host {
public:
  explicit Host(const char *path);
  ~Host();
  Host(const Host &) = delete;
  Host &operator=(const Host &) = delete;
  void Start(double rate = 48000);
  void Stop();
  void Set(clap_id id, double value);
  double Get(clap_id id) const;
  std::string Save() const;
  bool Load(const std::string &data);
  clap_process_status
  Render(uint32_t frames,
         const std::vector<const clap_event_header_t *> &events = {});
  const clap_plugin_t *plugin{};
  const clap_plugin_params_t *params{};
  const clap_plugin_latency_t *latency{};
  const clap_plugin_state_t *state{};
  std::array<float, 512> left{}, right{};
  unsigned restarts{}, latencyChanges{}, rescans{};

private:
  void *library_{};
  const clap_plugin_entry_t *entry_{};
  clap_host_t host_{};
  bool started_{};
};
clap_event_midi_t Note(uint32_t offset = 0, uint8_t velocity = 100);
void TestAudio(Host &host);
void TestState(Host &host);
void TestTiming(Host &host);
} // namespace clap_test
