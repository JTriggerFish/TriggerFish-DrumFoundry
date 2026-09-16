#pragma once
#include "../shared/event_queue.hpp"
#include "design_parameters.hpp"
#ifdef DRUMFOUNDRY_UI
#include "../shared/audio_tap.hpp"
#include "../shared/audition.hpp"
#include "ui/edit_history.hpp"
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
  ContactSpread,
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
  // Apply final parameter values together, then other events at this sample.
  void EventBatch(const clap_input_events_t *, uint32_t begin, uint32_t end,
                  bool notes = true) noexcept;
  void SetParameter(clap_id, double) noexcept;
  double Value(clap_id) const noexcept;
  double EditorValue(
      clap_id) const noexcept; // Main thread, includes queued edits.
  uint32_t LatencySamples() const noexcept { return latency_; }
  bool Save(const clap_ostream_t *);
  bool Load(const clap_istream_t *);
  // Main-thread edits share the sample-timed automation queue; structural
  // edits request host preparation instead of resetting a sounding voice.
  Json EditableDocument() const;
  void PrepareEditorPreset();
  void EditDocument(Json, int restoredPreset = -1);
  bool EditLiveDocument(Json);
  void EndDesignGesture();
  bool RestartPending() const { return restartQueued_.load(); }
  void SelectFactory(unsigned index);
  void EditPresentation(const Json &reference, const Json &analysis);
  double PreviewStrength() const { return previewStrength_.load(); }
  void SetPreviewStrength(double);
  unsigned DocumentRevision() const { return documentRevision_; }
  unsigned AutomationRevision() const { return automationRevision_.load(); }
  detail::Recipe DesignRecipe() const { return designRecipe_.load(); }
  const void *Extension(const char *) const noexcept;
  bool QueueEdit(clap_id, double) noexcept;
  bool QueueStrike(float velocity, float location) noexcept;
  bool QueuePanic() noexcept;
  void DrainEditor(const clap_output_events_t *, bool notes) noexcept;
  const clap_host_t *Host() const { return host_; }
  unsigned EditorErrors() const {
    return editorParams_->Dropped() + editorNotes_.Dropped() +
           editorNotificationErrors_.load();
  }
#ifdef DRUMFOUNDRY_UI
  std::unique_ptr<Editor> editor;
  std::shared_ptr<ui::EditHistory> editHistory{
      std::make_shared<ui::EditHistory>()};
  void EditLayout(const Json &positions);
  bool Audition(std::shared_ptr<const std::vector<float>>, unsigned rate,
                double gain);
  unsigned AuditionRate() const { return auditionRate_.load(); }
  host::TapRead ReadOutput(float *pcm, unsigned maximum) {
    return outputTap_.Read(pcm, maximum);
  }
  host::TapRead ReadVoice(float *pcm, unsigned maximum) {
    return voiceTap_.Read(pcm, maximum);
  }
#endif
  clap_plugin_t api{};
  bool processing{};
  bool active{}; // CLAP lifecycle synchronization guards accesses.
private:
  void Render(float *left, float *right, uint32_t frames) noexcept;
  void StrikeVoice(float velocity) noexcept;
  void RequestRestart() noexcept;
  struct DesiredState {
    Json document;
    std::array<double, ParameterCount> controls;
  };
  DesiredState CaptureDesired() const;
  void PublishDocument(const Voice &validated, int preset);
  void CancelEditorEdits(bool includeMonitor);
  void InitializeDesignParameters(const Json &document);
  void AcceptPendingDesign() noexcept;
  void OverlayDesignParameters(Json &document) const;
  void SetDesignParameter(clap_id, double) noexcept;
  void QueueDesignEdits(const Json &before, const Json &next);
  // One serialized CLAP writer; only the main-thread reader ever retries.
  struct DesignWrite {
    Plugin &plugin;
    explicit DesignWrite(Plugin &p) : plugin(p) {
      if (plugin.designWriteDepth_++ == 0)
        ++plugin.designSequence_;
    }
    ~DesignWrite() {
      if (--plugin.designWriteDepth_ == 0)
        ++plugin.designSequence_;
    }
  };
  unsigned designWriteDepth_{};
  std::atomic<uint64_t> designSequence_{};
  const clap_host_t *host_{};
  const clap_host_params_t *hostParams_{};
  const clap_host_latency_t *hostLatency_{};
  static constexpr std::size_t AllParameterSlots =
      ParameterCount + DesignCapacity;
  std::array<std::atomic<double>, AllParameterSlots> values_{};
  // Main-thread pending preset; never published into a still-sounding voice.
  std::array<double, DesignCapacity> desiredDesignValues_{};
  bool designPending_{};
  std::array<double, ParameterCount> audioValues_{};
  std::atomic<double> previewStrength_{.8};
  std::unique_ptr<Voice> voice_;
  output::Limiter limiter_;
  Json document_; // Main-thread-only saved/desired patch, never read in
                  // Process.
  int documentPreset_{};
  unsigned documentRevision_{};
  bool fixedBeater_{}; // Active recipe property, never inferred from a preset
                       // slot.
  uint32_t latency_{}, maximumFrames_{};
  double sampleRate_{48000}, masterGain_{}, masterTarget_{}, masterStep_{};
  double reductionHold_{};
  std::atomic<bool> restartQueued_{};
  std::atomic<unsigned> automationRevision_{};
  std::atomic<detail::Recipe> designRecipe_{detail::Recipe::Kick};
  std::array<bool, DesignCapacity> designGesturesMain_{},
      designGesturesAudio_{};
  std::unique_ptr<host::EventQueue<2048>> editorParams_{
      std::make_unique<host::EventQueue<2048>>()};
  host::EventQueue<> editorNotes_;
  struct PendingEdit {
    double value{};
    uint64_t serial{};
  };
  std::array<PendingEdit, AllParameterSlots>
      pendingEdits_{}; // Main thread only.
  uint64_t editSerial_{};
  std::array<std::atomic<uint64_t>, AllParameterSlots> acknowledgedEdits_{};
  std::array<std::atomic<uint64_t>, AllParameterSlots> cancelledEdits_{};
  std::atomic<unsigned> editorNotificationErrors_{};
#ifdef DRUMFOUNDRY_UI
  host::Audition audition_;
  host::AudioTap outputTap_;
  host::AudioTap voiceTap_;
  bool pendingStrike_{};
  std::atomic<unsigned> auditionRate_{};
#endif
};
} // namespace drumfoundry::clap_adapter
