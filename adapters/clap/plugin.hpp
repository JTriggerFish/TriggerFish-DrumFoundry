#pragma once
#include "../shared/event_queue.hpp"
#ifdef DRUMFOUNDRY_UI
#include "../shared/audition.hpp"
#endif
#include "output/limiter.hpp"
#include "runtime/voice.hpp"
#include <array>
#include <atomic>
#include <clap/clap.h>
#include <memory>

namespace drumfoundry::clap_adapter {
#ifdef DRUMFOUNDRY_UI
class Editor;
extern const clap_plugin_gui_t GuiExtension;
extern const clap_plugin_posix_fd_support_t FdExtension;
#endif
inline constexpr const char *PluginId = "com.triggerfish.drumfoundry";
inline constexpr std::array<const char *, 6> PresetNames{
    "Kick", "Snare", "Hi-hat", "Crash", "Ride", "Gong"};
// These host IDs are explicit and stable, never derived from DSP array order.
enum Parameter : clap_id {
  Preset = 100,
  Hardness,
  Implement,
  Location,
  Mute,
  Master,
  Protection,
  Reduction,
  Latency,
  ParameterEnd
};
constexpr std::size_t ParameterCount = ParameterEnd - Preset;
struct Control {
  const char *name, *group;
  double low, high, initial;
  bool stepped, readonly;
};
extern const std::array<Control, ParameterCount> Controls;
bool ValidValue(clap_id id, double value) noexcept;
extern const clap_plugin_descriptor_t Descriptor;
extern const clap_plugin_params_t ParamsExtension;
extern const clap_plugin_audio_ports_t AudioExtension;
extern const clap_plugin_note_ports_t NoteExtension;
extern const clap_plugin_latency_t LatencyExtension;
extern const clap_plugin_state_t StateExtension;

// Thin host shell: allocation/preparation happens only on the main thread;
// processing sees the current voice and sample-timed gesture controls only.
class Plugin {
public:
  explicit Plugin(const clap_host_t *host);
  ~Plugin();
  static Plugin &Get(const clap_plugin_t *p) {
    return *static_cast<Plugin *>(p->plugin_data);
  }
  bool Init();
  bool Activate(double rate, uint32_t minimum, uint32_t maximum);
  void Deactivate() noexcept;
  void Reset() noexcept;
  void OnMainThread() noexcept;
  clap_process_status Process(const clap_process_t *) noexcept;
  void Event(const clap_event_header_t *) noexcept;
  void SetParameter(clap_id, double) noexcept;
  double Value(clap_id) const noexcept;
  uint32_t LatencySamples() const noexcept { return latency_; }
  bool Save(const clap_ostream_t *);
  bool Load(const clap_istream_t *);
  // Main-thread document editing. Host restart publishes the prepared patch;
  // performance automation remains on its separate sample-timed path.
  Json EditableDocument() const;
  void PrepareEditorPreset();
  void EditDocument(Json);
  unsigned DocumentRevision() const { return documentRevision_; }
  const void *Extension(const char *) const noexcept;
  bool QueueEdit(clap_id, double) noexcept;
  bool QueueStrike(float velocity, float location) noexcept;
  bool QueuePanic() noexcept;
  void DrainEditor(const clap_output_events_t *, bool notes) noexcept;
  const clap_host_t *Host() const { return host_; }
  unsigned EditorErrors() const {
    return editorParams_.Dropped() + editorNotes_.Dropped() +
           editorNotificationErrors_.load();
  }
#ifdef DRUMFOUNDRY_UI
  std::unique_ptr<Editor> editor;
  bool Audition(std::shared_ptr<const std::vector<float>>, unsigned rate,
                double gain);
  unsigned AuditionRate() const { return auditionRate_.load(); }
#endif
  clap_plugin_t api{};
  bool processing{};
  bool active{}; // CLAP lifecycle synchronization guards accesses.
private:
  void Render(float *left, float *right, uint32_t frames) noexcept;
  void RequestRestart() noexcept;
  Json DesiredDocument() const;
  std::array<double, Reduction - Preset> DesiredControls() const;
  const clap_host_t *host_{};
  const clap_host_params_t *hostParams_{};
  const clap_host_latency_t *hostLatency_{};
  std::array<std::atomic<double>, ParameterCount> values_{};
  std::array<double, ParameterCount> audioValues_{};
  std::unique_ptr<Voice> voice_;
  output::Limiter limiter_;
  Json
      document_; // Main-thread-only saved/desired patch, never read in Process.
  int documentPreset_{};
  unsigned documentRevision_{};
  uint32_t latency_{}, maximumFrames_{};
  double sampleRate_{48000}, masterGain_{}, masterTarget_{}, masterStep_{};
  double reductionHold_{};
  std::atomic<bool> restartQueued_{};
  host::EventQueue<> editorParams_, editorNotes_;
  std::atomic<unsigned> editorNotificationErrors_{};
#ifdef DRUMFOUNDRY_UI
  host::Audition audition_;
  std::atomic<unsigned> auditionRate_{};
#endif
};
} // namespace drumfoundry::clap_adapter
