# Native workbench

The Visage editor is shared by CLAP and the standalone. Python remains optional
offline analysis/fitting; no browser, Node or Wasm renderer is introduced.

## Layout and interaction contract

- Single-row header: preset, patch name, limiter bypass, master and Settings.
  Limiter reduction/latency and device status stay visible at the bottom.
- Left: two independently laid-out control columns, excitation/output and
  resonance/bloom, with the T60 editor retained in its existing control area.
  Existing parameter sections appear in padded, rounded cards with distinct
  header bands and small functional accents. All controls remain visible;
  these groups do not add another level of collapsing or navigation.
- Right: spectrogram, strike surface with implement controls, wide
  modal editor. Resizable analysis/editor split; controls may scroll without
  moving playback and analysis out of reach.
  Resize dividers have thin visible rules with wider invisible grab areas.
  Scrollbars are 4 px at rest and 8 px on hover, independent of text size.
- Bottom: one compact row for limiter/device status and persistent errors.
  Hover for the full status/error text when it does not fit.
- Presets menu: separate factory and user sections, plus Save preset as,
  Import and Export. User subfolders become nested menus. Former snapshots appear as user presets in the same
  library; no user files are moved or deleted. Saving creates a new stored
  version without restarting the voice. User presets show their save time.
  Settings → User preset folder selects a persistent library shared by standalone
  and CLAP. The default remains the application's local `fits` folder.
  Calibrations with reference attachments are local user presets only: neither
  embedded in the binaries nor installed with factory assets.
- Preserve double-click defaults, curve editing, modal painting/harmonic tools,
  reference-only colour normalization, mirrored alignment and black-centred
  difference display. No automatic audio normalization.

Use Visage buttons, menus, text editors and scrolling directly. The library has
no stock audio slider: a small conventional horizontal Frame-based control is
needed. Custom DSP editors and graphs use Visage drawing, not browser widgets.
The default Classic palette restores the original workbench colours; LazyVim
remains a built-in choice. [JSON colour schemes](colour-schemes.md)
can replace it through Settings without changing presets or synthesis.
All instruments share a single graphical final output EQ: bypass, high-pass,
one variable-width colour bell and low-pass, with the live output spectrum behind it.
Peak Q is always visible as an editable readout (0.1–20; higher is narrower).
Scroll over the bell handle to change width, or Shift-scroll for fine adjustment.
Double-clicking the bell resets frequency, gain and Q. Existing presets keep
their previous broad response through the Q=0.7 default. Q uses the same live,
5 ms coefficient smoothing and CLAP automation path as frequency and gain;
the response plot uses the actual DSP coefficients, with no extra EQ latency.
The graph uses a dark inset and subdued spectrum, with outlined coloured handles
and a bright response curve. Hover/drag shows the handle's value. Handles and
click-to-edit numeric readouts remain editable when bypassed; editing never enables the EQ
implicitly. The one-click enable button and graph status make bypass explicit.
There are no section-specific EQs or multiband controls in instrument patches.
The kick places contact/output in the first column and thump/resonance/tension
in the second. Old bypass/radiation drum fits convert to the shared EQ names;
active multiband fits are rejected explicitly rather than silently retuned.

The analysis toolbar uses compact wrapping controls below the plot, matching
the web workbench ordering. Above it, optional reference selection stays separate
from the instrument's strike controls. Factory presets start with None and a
single model plot. Calibrations retain their explicit reference gain.
The spectrogram occupies the full analysis view below its legend; no separate
waveform lanes are displayed. Mirrored pan/alignment and split gestures remain.
Each horizontal comparison pane has its own time ticks, including its starting
time at any divider position. Narrow panes retain that label without overlap.
The strike pad has a contrasting surface, directional axes, in-pad legends and
a last-click crosshair/velocity readout. Up means stronger; right means a harder
beater for kicks, or a strike nearer the edge for other instruments. The whole
pad remains playable: the drawn axes span the full input range, with clicks in
the surrounding margins clamped to their endpoints. The marker uses the same
rectangle and preserves its normalized position when resized.
The strike pad sits beside implement/character controls on wide panels and
above them on narrow ones. Drag the vertical divider to resize the control
area, or the horizontal divider below analysis to resize the plot. Settings → Layout
can hide the spectrogram or modal editor independently, select one control
column, and reset the layout. Narrow control areas use Excitation/Resonance tabs;
wider areas retain both columns. Modal and series tools wrap rather than overlap.
Layout choices are presentation-only and are stored with saved fits/host state.
Selected Layout options have a persistent highlighted background. Text size offers
Small (the original 13 px), Medium (15 px) and Large (17 px), independent of OS DPI.
Parameter captions and values stay on one line when they fit; only genuinely
narrow rows expand. Toolbars wrap and narrow control areas use tabs.
Text preferences are per editor, including native menus and text-entry fields.
The standalone and resizable CLAP editor support windows down to 900 × 600.
Regression checks cover 900 × 600 through 3200 × 1800 and all factory instruments.

The play triangle beside a selected reference auditions it; None has no play
button. Reference selection remains accessible with the
spectrogram hidden. Previous/Next steps through WAVs in the current folder,
preserving the explicit reference gain. No recording is normalized on selection.

Standalone launches automatically start the saved audio/MIDI configuration.
First launch or a device-open failure opens Settings; there is no silent fallback
to another device. A stopped engine is identified in the bottom status, with
Settings highlighted. Striking or auditioning while stopped opens Settings,
where **Apply & start** activates the selected device. Offline preview rendering
does not imply that a device is running. The click-to-CLAP audio path is tested
at 128 samples with hardware-free output capture; this does not test a driver.

## Iterative delivery

1. Optional pinned Visage build, shared editor foundation, native performance
   controls, standalone window and CLAP embedding/lifecycle.
2. Native settings/device selection and shared patch editing/preset save/load.
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
  double-click; the saved device starts automatically (first launch asks for a
  selection). ASIO discovery
  reads driver registration names, never probes unrelated vendor DLLs.
  Apply remembers API/device/MIDI/rate/buffer between sessions and next launch
  attempts that same configuration. MIDI failures leave audio available and display the
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
- User presets persist as independent fit JSON documents in the configured
  folder (by default application-data `TriggerFish/DrumFoundry/fits`). Each records its parent
  ID and retains reference identity and analysis settings. Former snapshots are
  listed in the same User presets menu. Save preset as names a new version;
  Import validates and copies a new version into this library before loading it;
  Export writes a portable copy. File selection uses an in-window Visage picker.
  Writes validate first and exclusively create a new file, never overwriting a
  previous fit. Native tests check round-trip persistence and overwrite rejection.
- Bloom timing and Size meta are native in-window tools, opened from the
  Bloom and Resonance sections respectively (Size is not an output EQ tool).
  Sliders preview ordinary controls; release applies the
  patch, and Cancel restores the captured baseline. Bloom timing retains the
  web formula (rate × 2^-p, excitation tilt − 6p, centre × 2^(-p/4)). Size meta
  retains the old explicit design endpoints, not the current calibrated fits;
  neutral is the authoritative descriptor defaults for its affected controls.
  Endpoints and interpolation live in `engine/editing`, with validation tests.
  Neither tool adds voice parameters. The native hold-decay optimizer is described below.
- Hardness, spread, location, pedal and mute share two untitled control columns
  beside the strike pad. They stack on compact layouts. Spread is inactive only
  for metallic sticks/mallets, where the engine does not use it.
  Freeze strike opens an editor-local popup for a fixed velocity/location pair;
  a highlighted button indicates frozen pad input. MIDI is unaffected, and a
  change between kick hardness and strike-location axes clears the freeze.
  Native pad strikes retain continuous velocity; MIDI remains velocity/127,
  with no compression. The latest strike updates the offline-render gesture.
  Performance defaults come from the loaded fit. Tests compare initial and
  repeated native pad output against an independent direct Voice render.
- Routing is collapsed above the two left control columns. Expand for a compact
  diagram, then click it for a larger in-window editor with native route
  switches and a **Modules…** menu. [Rim contact](topology-modules.md) can be added,
  bypassed or removed on metallic and membrane bodies. Green connections are
  body-energy attachments, not audio paths. Required audio routes are locked by the compiled contract, not by editable
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
offline preview before release. Structural edits replace the voice on release;
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
`build/native/tests/analysis_preview_bench presets/factory/gong.fit.json`
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

Choose any local WAV library in Settings → Reference library
folder. The native UI browses the folder hierarchy directly; it does not need
`catalog.json`, Python or the old repository. Recordings are never packaged or
committed. Selecting a reference does not alter synthesis settings or normalize
gain. The optional attachment and missing-file behaviour are specified in
[Reference libraries](reference-libraries.md).

The reference play triangle auditions the exact PCM represented by its plot,
through the host master and optional 1 ms limiter. File gain is explicit; model
gain is already in its rendered PCM. A separate off-thread libsamplerate sinc
conversion prepares audition buffers for the active device rate without changing
analysis samples. Live pad/MIDI strikes interrupt audition and process the actual
Voice in real time. There are no header play/stop controls. Bounded immutable slots avoid audio-
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

Every instrument uses the same final radiation EQ and a three-handle
EQ plot with the live output as its background. High-pass and low-pass handles
move horizontally; the colour handle moves frequency and gain. Duplicate
sliders are replaced by four compact, clickable numeric readouts inside the
slightly taller graph (Enter applies, Escape cancels). A compact EQ on/off button
is inside the graph header. Output level remains separate above the graph; it
balances the instrument against references, unlike master listening volume. Bypass
leaves a flat total response; double-click resets the selected handle. There is
no additional Q, gain or hidden shaping parameter. The curve uses the same DSP
bandwidths in every recipe (Butterworth cuts and a broad colour band, Q 0.7).
The
parameter builder and biquad designs run at the live device rate (otherwise the
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

The bloom section keeps **Bloom timing** and **Hold decay** side by side.
Hold decay is on initially; while busy its button shows Cancel and elapsed time,
with render count and results in the tooltip. After a bloom/excitation gesture,
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

## Modal selection gestures

In **Select & move**, drag empty plot space to draw a selection rectangle
around mode handles. Drag anywhere inside the highlighted group rectangle or
a selected packet's yellow fill to move the group in frequency
and prominence. Frequency ratios and relative dB levels are preserved, including
when the group reaches an editor boundary. With harmonic snapping enabled, the
grabbed handle snaps; the group's other intervals stay intact.

- Shift-click or Shift-drag an empty area adds to the selection. Shift while
  moving a handle gives fine adjustment.
- Ctrl+scroll (Command+scroll on macOS also works) widens/narrows the selected
  modes' sidebands using their existing local noisiness values. Plain scrolling
  edits only the active handle. Widths remain within their existing limits.
- Delete/Backspace removes the selection; Escape cancels an in-progress move
  or paint gesture and restores its starting modes, then clears the selection.
- Double-click empty space to insert a mode, or a circular handle to delete it.
  A stem below a handle is not a double-click deletion target. A full bank
  reports its capacity error rather than replacing another mode.

Selected modes are highlighted; the active handle has a light centre. The lower
numeric controls edit that active handle, not the whole selection. The tool
button's tooltip lists gestures. The brush tools retain their separate painting
behaviour. Selecting modes never changes synthesis or triggers a new render.

## Undo and redo

The header buttons keep up to 128 edits per instrument instance, including
modal painting/moving/deleting, curves, EQ, routing, macros, performance controls,
reference choices and preset loading. A drag is one step. Accepted automatic
Hold decay compensation merges with its initiating edit when it is still the
latest edit. New edits discard the redo branch; unchanged clicks do not.
Playback, MIDI strikes, viewport/layout changes and file writes are not undone.

Windows/Linux: **Ctrl+Z**, **Ctrl+Y** or **Ctrl+Shift+Z**. macOS: **Cmd+Z** and
**Cmd+Shift+Z**. These shortcuts are editor-local; text fields retain their own
undo. In a DAW, focus the plugin editor first. A host may intercept shortcuts;
the buttons remain available. This is a separate instrument-edit history, not
the DAW's project undo history.

History survives closing/reopening the editor in the same plugin instance. It
is not saved in presets or projects; loading host state or an externally selected
factory preset clears it. Restoring edits preserves unrelated live performance
values and the current spectrogram view. Live-safe changes use the parameter
queue; structural changes use the host preparation lifecycle.
Queued performance edits retain their requested value until the audio thread
acknowledges them, so consecutive gestures remain distinct undo steps. Preset
undo restores the host selector alongside the edited document, not factory
defaults; stale queued strike-control edits cannot overwrite that restored patch.

## Live controls

EQ handles/readouts, EQ bypass, output/source levels, metallic bloom controls and
the T60 curve update while playing, including during a drag. Membrane resonance
decay and tension controls also retain the sounding voice. Contact and thump/FM
envelope edits shape the next hit without cutting off existing notes. EQ and
gain changes have short click-reducing ramps, with no added audio latency.

Modal movement, prominence, widths, allocation, packet texture, drift and shimmer
now update while dragging without restarting the ringing voice. Kick resonances
and membrane pitch/character use the same prepared-publication path. Parameter
design runs outside the callback; one sounding bank retains its state. New
handles begin silent until excited; deleting a handle removes its stored energy.
Routing and observation delay still prepare on release. Spectrogram
previews are separate background work. One drag remains one undo step, and live
undo/redo does not restart the audio device.

The lightweight live-safe controls are also exposed to CLAP host automation with stable
IDs, sample-timed playback and begin/end gestures for recording UI drags. Host
automation updates visible readouts without rebuilding the panel. Structural
controls, prepared modal edits and CLAP per-note modulation remain outside this automation surface;
see [clap.md](clap.md).

## Modal bandwidth

Metallic modal centres and series generation span up to 20 kHz. The audio engine
keeps oscillators below 0.48 times its sample rate (including packet side modes).
Kick resonance limits remain unchanged. The modal graph, typed frequency,
harmonic snapping and group dragging use each recipe's parameter range.

The T60 editor also spans 20 kHz. Select its right square endpoint to move its
frequency between 15 and 20 kHz, or drag it horizontally; its existing frequency
slider shows the value. Decay stays flat beyond the endpoint. The explicit JSON
parameter `body_decay_frequency_7` defaults to 15000 when absent, preserving old
curves and their six optional interior knots without rescaling or migration.
Move the endpoint right before adding a knot beyond it. Endpoints cannot be
deleted; the low endpoint remains at 40 Hz.

## Dependency references

- [Visage](https://github.com/VitalAudio/visage/tree/828037000d0893647ab29b66ae9c4a241c90f671):
  MIT, GPU graphics, native windows, standard widgets and plugin embedding.
- [CLAP GUI contract](https://github.com/free-audio/clap/blob/195b42a004144fab0b3cf95e9c067187d15365b7/include/clap/ext/gui.h):
  lifecycle, parent embedding, scale and resizing.
- [dr_wav](https://github.com/mackron/dr_libs): pinned native WAV decoder.
- [PicoSHA2](https://github.com/okdshin/PicoSHA2): pinned native file hashing.
- [libsamplerate whole-buffer API](https://libsndfile.github.io/libsamplerate/api_simple.html):
  pinned BSD-licensed sinc conversion, only for off-thread audition preparation.
