# Native workbench migration

The Visage editor is shared by CLAP and the standalone. Python remains optional
offline analysis/fitting; no browser, Node or Wasm renderer is introduced.
The reference is TriggerFish-VCV's `workbench/web`, preserved there unchanged.

## Layout and interaction contract

- Header: presets/reference selection, playback, master, persistent limiter
  reduction/latency/bypass and standalone settings.
- Left: two independently laid-out control columns, excitation/output and
  resonance/bloom, with the T60 editor retained in its existing control area.
- Right: waveform/spectrogram, strike surface with implement controls, wide
  modal editor. Resizable analysis/editor split; controls may scroll without
  moving playback and analysis out of reach.
- Bottom: named snapshots, fit save/load and persistent errors (never swallowed).
- Preserve double-click defaults, curve editing, modal painting/harmonic tools,
  reference-only colour normalization, mirrored alignment and black-centred
  difference display. No automatic audio normalization.

Use Visage buttons, menus, text editors and scrolling directly. The library has
no stock audio slider: a small conventional horizontal Frame-based control is
needed. Custom DSP editors and graphs use Visage drawing, not browser widgets.
Do not redesign the control surface or change presets during this port.

## Iterative delivery

1. Optional pinned Visage build, shared editor foundation, native performance
   controls, standalone window and CLAP embedding/lifecycle.
2. Native settings/device selection and shared patch editing, save/load/snapshots.
3. Port recipe-aware parameter grouping and native modal/T60/meta editors.
4. Reference selection/playback and off-thread native rendering/FFT/STFT,
   GPU presentation, comparison/alignment and interactive analysis.

Review/test each chunk, commit and push to dev, and check all four CI platforms.
Incomplete panels must say so; placeholder graphics are not measurements.
Audio callbacks never touch Visage, allocate display data or wait for rendering.

## Progress

- Foundation: shared native content, stock Visage buttons/popups, conventional
  sliders with double-click reset/fine drag, strike pad, limiter/status/error
  readouts and standalone `--gui` path. `--ui-smoke` captures a real GPU frame and
  closes without opening devices. Interaction tests do not require a display.
- CLAP embedding: shared content via the standard GUI extension, native Windows/
  Cocoa/X11 parents, host resizing and Linux FD dispatch. UI controls use bounded
  queues; the audio/flush consumer applies them and emits host parameter events.
  Manual strikes wait for Process, including when initially inactive. The
  `--clap-ui-smoke` embedding/capture test is currently verified on Windows.
- `cmake/visage-compat.cmake` fixes the pinned Windows parent-procedure lookup
  when a Visage editor is embedded in a Visage host. Generated source only; no
  upstream checkout mutation. The real embedding test caught this dispatch bug.
- Standalone settings: in-window modal panel with API/device/MIDI/rate/buffer
  menus, explicit Apply/start and release. UI builds open this workbench on
  double-click; they do not open an audio device until selected. ASIO discovery
  reads driver registration names, never probes unrelated vendor DLLs.
  Hardware-free smoke captures include the settings overlay; validation uses
  fake device callbacks in native tests. Real device handling is the tested
  RtAudio/RtMidi host, not a second implementation.
- Patch editing: native descriptor-driven controls, two independently scrollable
  columns and the web workbench's low-range tapers. JSON edits validate before
  publication; failed edits report an error and restore the valid document.
  This first structural-edit path applies on slider release and resets the
  voice through the host restart lifecycle. It does not yet promise uninterrupted
  tails during structural edits. Performance gestures remain real-time.
- `engine/editing` owns native document/value/scaling/presentation helpers,
  separately from DSP and Visage. Tests check all six fits, parameter round trips,
  taper bounds and transactional rejection.
- Native modal/T60 editors now replace their scalar rows. Mode addition/deletion,
  dragging, noisiness width, same-direction prominence painting, harmonic guide,
  snapping and explicit harmonic/membrane series generation use ordinary JSON
  parameters. Unsupported recipes retain their body controls rather than exposing
  nonfunctional painted modes. T60 uses ERB/log-time DSP interpolation with the
  one-second-knee display, two permanent endpoints and up to eight total knots.
  Series/taper/document and headless pointer-gesture tests accompany the port.
- Right-hand content scrolls as one column; analysis, strike and modal design
  keep their ordering. The divider below analysis changes its height; larger
  windows expand the analysis/editor area to use available height. Its division
  is stored with the view, while the two left control columns remain independent.
  A fixed-velocity Strike button complements the continuous 2D playing surface.
- Named snapshots persist as independent fit JSON documents under the user's
  application-data `TriggerFish/DrumFoundry/fits` folder. Each records its parent
  ID and retains reference identity and analysis settings. Saved snapshots can
  be restored after restarting. Save/load uses a small in-window Visage path/
  folder picker; no shell command or browser filesystem permission is involved.
  Writes validate first and exclusively create a new file, never overwriting a
  previous fit. Native tests check round-trip persistence and overwrite rejection.
- Bloom timing and Size meta are native in-window tools, opened from their
  parameter sections. Sliders preview ordinary controls; release applies the
  patch, and Cancel restores the captured baseline. Bloom timing retains the
  web formula (rate × 2^-p, excitation tilt − 6p, centre × 2^(-p/4)). Size meta
  retains the old explicit design endpoints, not the current calibrated fits;
  neutral is the authoritative descriptor defaults for its affected controls.
  Endpoints and interpolation live in `engine/editing`, with validation tests.
  Neither tool adds voice parameters. The hold-decay optimizer remains to port.
- Gesture spread and fixed audition velocity are beside the strike controls.
  Native pad strikes retain continuous velocity; MIDI remains velocity/127,
  with no compression. The latest strike updates the offline-render gesture.
  Performance defaults come from the loaded fit. Tests compare initial and
  repeated native pad output against an independent direct Voice render.

## Native analysis preview

The optional UI target includes WAV decoding, reference SHA256 verification,
offline native Voice rendering and centred STFT analysis on a cancellable worker.
It never reads the live voice. Reference decode/hash/STFT results are cached by
file identity, channel and transform. Performance edits are briefly debounced;
the last completed plot stays visible until a replacement is ready.

Visage GPU heatmaps show mirror (default), side-by-side, stacked, individual or
difference views. The colour ceiling comes only from the reference; without a
reference it stays at 0 dBFS. No PCM normalization occurs. Difference is model
minus reference in dB, black at zero, amber for excess and cyan for missing energy.
Both sides use the same reference floor. Wheel pans both sides reversibly;
Ctrl-wheel zooms time, Alt-wheel zooms frequency, Shift-drag aligns either side,
and the divider is draggable. Hover shows time/frequency and both spectra's bin
levels. FFT's menu includes overlap choices; the view menu includes difference
range. Reference-dependent colour scaling does not change when model settings do.
FFT size, window, render duration, colour range, explicit reference gain and mono/
left/right channel selection are native controls. Saved fits retain the channel.
Snapshots and CLAP state retain view, zoom, alignment, render length, channel,
reference and transform. UI-only metadata changes mark host state dirty without
restarting the voice. Structural edits retain that current presentation metadata.

The optional, one-time `tools/import_workbench_references.py` helper copies the
old workbench's curated allow-list into the user's application-data
`TriggerFish/DrumFoundry/references` folder. It refuses conflicting existing files.
The native UI reads `catalog.json` and WAV files directly; Python and the old
repository are not runtime dependencies. Recordings are never packaged or
committed. A native hierarchical menu offers the curated instrument/cell grid,
and Other WAV opens the native picker. Selecting a reference does not secretly
alter synthesis settings or normalize gain.

Play reference and Play render audition the exact PCM represented by the plots,
through the host master and optional 1 ms limiter. File gain is explicit; model
gain is already in its rendered PCM. A separate off-thread libsamplerate sinc
conversion prepares audition buffers for the active device rate without changing
analysis samples. Live pad/MIDI strikes interrupt audition and process the actual
Voice in real time. Stop halts both paths. Bounded immutable slots avoid audio-
thread allocation, reference-count destruction, locks or filesystem work. Native
host tests verify gain, stop, rate mismatch and allocation/deallocation freedom.
Neither offline rendering nor audition preparation opens a device; standalone
Settings still owns device selection, while CLAP uses its host's output.

## Dependency references

- [Visage](https://github.com/VitalAudio/visage/tree/828037000d0893647ab29b66ae9c4a241c90f671):
  MIT, GPU graphics, native windows, standard widgets and plugin embedding.
- [CLAP GUI contract](https://github.com/free-audio/clap/blob/195b42a004144fab0b3cf95e9c067187d15365b7/include/clap/ext/gui.h):
  lifecycle, parent embedding, scale and resizing.
- [dr_wav](https://github.com/mackron/dr_libs): pinned native WAV decoder.
- [PicoSHA2](https://github.com/okdshin/PicoSHA2): pinned native file hashing.
- [libsamplerate whole-buffer API](https://libsndfile.github.io/libsamplerate/api_simple.html):
  pinned BSD-licensed sinc conversion, only for off-thread audition preparation.
