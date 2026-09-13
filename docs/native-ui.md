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

The analysis toolbar uses compact wrapping controls below the plot, matching
the web workbench ordering. Above it, reference corpus/articulation/velocity/take
selection stays separate from the instrument's strike controls. Catalogue
`reference_gain_db` is preserved when selecting a different recorded layer.
Waveforms have labelled, forward-time lanes and a shared reference amplitude
scale; mirroring applies only to the spectrogram, including its pan gestures.
The strike pad sits beside implement/character controls, not above full-width
sliders. Layout regression checks cover 1000, 1440 and 3200 pixel windows.

Standalone device selections are restored without acquiring an exclusive audio
device. A stopped engine is explicitly labelled beside playback with a
**Start audio** button. Striking or auditioning while stopped opens Settings,
where **Apply & start** activates the selected device. Offline preview rendering
does not imply that a device is running. The click-to-CLAP audio path is tested
at 128 samples with hardware-free output capture; this does not test a driver.

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
- Right-click any slider for native numeric entry in its physical units (Hz,
  seconds, dB, etc.). Enter applies; Escape cancels. Out-of-range, nonfinite and
  fractional count values report an error without changing the sound. Double-
  click still resets and Shift-drag adjusts finely. External value changes cancel
  an unfinished entry so it cannot later overwrite a newly loaded setting.
- Delayed, non-interactive help explains the audible effects of packet spacing,
  beating, drift, shimmer, blur and bloom. Standard Visage hover/click behaviour
  remains intact. Generic controls also explain reset, fine adjustment and
  numeric entry. Rebuilding a preset dismisses stale help.
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
  Apply remembers API/device/MIDI/rate/buffer between sessions, without opening
  devices on next launch. MIDI failures leave audio available and display the
  named port plus backend error in a wrapped, scrollable message. Details and
  the preference-file location are in [standalone.md](standalone.md).
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
- Routing is collapsed above the two left control columns. Expand for a compact
  diagram, then double-click it for a larger in-window editor with native route
  switches. Required routes are locked by the compiled contract, not by editable
  JSON hints. Invalid disconnections leave the patch unchanged. There are no
  extra edge gains. Moving boxes saves only validated node positions, marks host
  state dirty and neither restarts nor retunes the voice. The canvas fits the
  saved kick/snare layouts rather than clipping them to the metallic layout.

## Native analysis preview

The optional UI target includes WAV decoding, reference SHA256 verification,
offline native Voice rendering and centred STFT analysis on a cancellable worker.
That worker owns its own voice. Reference decode/hash/STFT results are cached by
file identity, channel and transform. Reference audition resampling is cached
separately by the decoded source and device rate. Performance and design edits
are briefly debounced: a roughly 130 ms pause during a drag can update the
offline preview before release. The live voice is still replaced only on release;
the previous spectrogram stays behind an advancing grey write edge. New audio
and centred FFT frames arrive incrementally, rather than waiting for a complete
render or showing a progress bar. The renderer is unthrottled, not paced by the
audio device; it publishes deltas approximately every 50 ms. Reference metadata
and playback become available before the model render completes.

MIDI and pad strikes retain their actual velocity for subsequent edit previews.
They do not launch an offline re-strike: a second bounded SPSC tap copies the
actual mono instrument output **before monitor master/limiter**, with sample-
aligned strike markers. A separate worker incrementally analyzes that stream.
The first strike starts a display-length capture; repeated strikes accumulate
on the same timeline with their actual overlaps. After the window fills, the
next strike starts another pass over the previous plot. Editing parameters
switches back to the faster-than-realtime preview. Reference audition is not
mixed into the instrument capture. A completed live capture can be replayed;
unfinished or stale model renders cannot silently substitute for it.

Both paths share the same streaming FFT/window implementation. Centred frames
wait for their right-hand samples (half a window: about 43 ms at 4096/48 kHz);
only the true end of a capture is zero-padded. This is **display latency only**,
not extra audio latency. GUI polling is approximately 30 Hz. No FFT, allocation
or UI synchronization was added to the audio callback. A display overrun reports
a gap and waits for a fresh strike, without blocking audio.

Heatmap updates touch only the newly written model time strip. Reference pixels
and the unwritten old tail retain their colours. An interrupted preview's
background is flattened, so repeated edits do not retain unbounded history.
Zoom/reset continues to use the selected render length, not the partially filled
buffer length.

Local MinGW release measurements (2026-09-13, factory fits, 8 seconds at 48 kHz,
4096/512 Hann, no reference-file load): gong approximately 1.53 s, crash 2.94 s;
first published chunk approximately 16–26 ms. At 1200×500, full heatmap preparation
was approximately 13–18 ms; a 33 ms live strip approximately 0.55 ms. These are
CPU-stage timings, not end-to-end display measurements; add edit debounce,
polling, initial reference decode and screen refresh. Reproduce with
`build/native/tests/analysis_preview_bench presets/gong_calibration.fit.json`
(append `.exe` on Windows) after `./dev.ps1 ui-test`.

Regression tests compare whole/streamed FFTs for every supported window, capture
raw host PCM with strike markers, exercise cancellation and verify that two
strikes remain one unmodified live capture through the actual analysis panel.

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

The Output section also shows a live 8192-sample Hann spectrum of the actual
post-master/limiter dual-mono output, including reference audition. Its fixed
0 to -96 dBFS/bin scale never follows the signal level. Peak pooling into display
columns retains narrow ridges. The audio callback only copies PCM to a bounded
SPSC tap; it never performs FFTs, allocates or waits. A stalled display drops its
own data and discards stale backlog. Windowing/FFT happens on the GUI thread at
the existing 30 Hz refresh rate, with cached FFT state; live and offline
spectrograms use independent workers. Tests cover tone/DC calibration, block-size
invariance, concurrent tap wraparound and equality to actual host output.

Where the recipe has the final radiation EQ, that display becomes a three-handle
EQ plot with the live output as its background. High-pass and low-pass handles
move horizontally; the colour handle moves frequency and gain. The ordinary
four sliders remain visible and synchronized, including numeric entry. Bypass
leaves a flat total response; double-click resets the selected handle. There is
no additional Q, gain or hidden shaping parameter. The curve uses the same DSP
parameter builder and biquad designs, at the live device rate (otherwise the
preview-render rate). Tests compare it with the actual filter's impulse response
at four sample rates and exercise graph-to-JSON editing. The curve axis is EQ dB;
the background retains its separate fixed 0 to -96 dBFS/bin scale.

Factory presets are explicit load actions, including reselecting the currently
edited preset. The title shows the actual loaded fit name; recipe-specific
strike/mute behavior follows the document, not the factory slot it originated
from. Raw patches gain explicit default strike and analysis metadata on import;
their parameters and rendered sound are preserved. State saves capture the
selected document and host controls together, including during rapid selection.

Zoomed-out heatmaps retain the strongest bin/frame within each pixel instead of
skipping narrow ridges or attacks between sample points. The hover readout stays
an individual STFT-bin measurement. Pooling is identical on both sides and does
not change colour scaling, PCM or fitting losses. Invalid saved views are checked
before mutating the existing analysis display.

## Hold decay

The bloom section retains the optional **Hold decay** switch (on initially),
with Cancel, elapsed time and render count. After a bloom/excitation gesture,
an independent native worker attempts to preserve the prior 1–6 second tail
while protecting the edited 0–450 ms attack/bloom. A new sound or performance
edit cancels it; changing zoom or moving routing boxes does not. Accepted changes
update the visible T60 knots and, only if it was not edited, concentration
dependence. A rejection leaves the user's edit intact. No new knots, hidden
envelopes, level matching or runtime solver state are introduced.

The migrated bounded least-squares method uses the actual C++ Voice, six-second
renders, a 4096-point symmetric Hann, 30 ms hops and 24 log bands from 80 Hz to
16 kHz (limited below Nyquist). It has three iterations, finite differences,
bounded T60 changes (at most ×2/÷2), attack constraints and an independent-seed
check. This is a design aid, **not a claim of perceptual fit quality**. Tests cover
known damping recovery, FFT power calibration, unreachable targets, independent
seed rejection, unchanged caller documents and render-block cancellation.
The implementation is in `workbench/decay_hold`; it has no browser/Python runtime
dependency and is excluded from headless engine builds.

## Dependency references

- [Visage](https://github.com/VitalAudio/visage/tree/828037000d0893647ab29b66ae9c4a241c90f671):
  MIT, GPU graphics, native windows, standard widgets and plugin embedding.
- [CLAP GUI contract](https://github.com/free-audio/clap/blob/195b42a004144fab0b3cf95e9c067187d15365b7/include/clap/ext/gui.h):
  lifecycle, parent embedding, scale and resizing.
- [dr_wav](https://github.com/mackron/dr_libs): pinned native WAV decoder.
- [PicoSHA2](https://github.com/okdshin/PicoSHA2): pinned native file hashing.
- [libsamplerate whole-buffer API](https://libsndfile.github.io/libsamplerate/api_simple.html):
  pinned BSD-licensed sinc conversion, only for off-thread audition preparation.
