# Native engine boundary

`engine/tfdsp/` contains reusable DSP and the compiled instrument recipes.
`engine/parameters/` contains named controls and their mapping into DSP settings.
`engine/patch/` owns JSON parsing, recipe topology and parameter ownership.
`engine/runtime/` owns independently allocated voices. There is no process-global
shared voice registry, and independent renderers can run on separate threads.

```text
JSON fit/patch --> validate + prepare --> owned native Voice
                                            |
                       timed strike ------> process --> mono PCM
                                            ^
                              C API / optional CLAP adapter
```

Parameter descriptors are authoritative for defaults, ranges and serialized
controls. The runtime supports both prepared structural changes and
state-preserving scalar automation. There is no implicit gain matching,
limiter, sample-rate conversion or output EQ beyond
the controls explicitly present in the patch. EQ can be disabled in the patch.
Metallic voices also offer an independent, direct-only [strike noise accent](strike-observation.md);
its level defaults to zero and it never drives the modal body.
An optional [modal tail damping](modal-tail-damping.md) control adds passive,
friction-like loss to metallic resonances; zero preserves exponential damping.

Preparation allocates and must run off the audio thread. A failed JSON load
leaves the existing voice intact. Loading a valid replacement resets its state;
replacement is distinct from the state-preserving prepared modal edits below. `Reset`, typed
`Trigger` and `Process` preserve the original energy/restrike semantics. One
voice must not be called concurrently. C ABI strings are library-owned and copied
immediately by the Python wrapper; NumPy owns each output buffer.

Topology is a fixed compiled recipe, not a general graph interpreter. The JSON
contract rejects wrong owners, invalid values, duplicate keys/IDs/routes, unknown
types/versions, missing/disabled required connections and disconnected outputs.
Missing node parameters are expanded from the authoritative C++ defaults during
loading. Returned/saved documents therefore contain every effective parameter;
supplied values are preserved, not replaced. Explicit null/invalid objects are
errors. Expanded documents remain independent of later default changes.
Decimal endpoints are checked at DSP float precision, with no broad tolerance
or clamping. Choice/boolean values must still be exact integers in JSON.
Preparation, editing and host automation share `parameters/validation.hpp`;
the contract belongs to the engine, not a host adapter. This C API is a
development interface, not a released
binary compatibility promise.

Optional native host-output protection is implemented separately in
`drumfoundry_output`; see [output-limiter.md](output-limiter.md). It adds a fixed
1 ms only when explicitly used by a host. Raw rendering and fitting remain
unlimited. The native UI exposes bypass, latency and gain reduction.

## Remaining runtime work

- Series generation, Size meta and bloom timing share helpers in `engine/editing`.
  Python series generation calls the same C++ generator through the C ABI;
  it has no separate stretching formula. The editor rejects range overflow;
  fitters may explicitly request truncation of the generated series.
  Hold-decay compensation now runs natively in `workbench/decay_hold`, with known
  envelope, calibration, rejection and cancellation tests. Expanded presets do
  not require these design-time tools to render; do not reimplement them
  independently in Visage/Python.
- Extend host automation to the prepared modal/texture editing path, and finish
  remaining editor integration. The shared Visage editor, reference analysis, fit storage,
  routing and graphical device management are described in [native-ui.md](native-ui.md).
  CLAP integration is documented in [clap.md](clap.md), and the audio/MIDI host
  in [standalone.md](standalone.md).

The current runtime still embeds storage for each available recipe inside an
owned session. This is not a global instance limit, but can be reduced to active
recipe storage before polyphonic/multi-instrument host integration.

## Live control edits

The metallic body also has optional [hi-hat rim contact](hi-hat-contact.md):
passive modal collision impulses, moving-pedal work and live MIDI CC4 openness.
It defaults off in the component but is enabled in the factory Hi-hat, the
startup instrument. It shares the existing modal energy, rather than adding noise.

`runtime/live_controls` validates design edits on the main thread without creating
a replacement voice. UI changes enter a bounded single-producer/single-consumer
parameter queue, drained at the next audio block. CLAP host automation enters
the same runtime setters at the event's sample offset. The adapter validates
the final same-sample curve and orders coupled scalar edits safely.
`Voice::StageParameter` validates each scalar state; `FlushParameters` applies
the completed batch before the next sample or strike. No audio-thread JSON,
allocation, lock, voice reset, or device restart is involved. Preset replacement cancels stale queued
edits. The adapter overlays current parameter values into saved JSON;
`Voice::Document()` remains the original configuration, not an audio-thread mirror.

Final EQ/bypass and gains transition over 5 ms (coefficient/gain smoothing, **not
latency**). Filters retain their history and stay warm while bypassed. Modal
damping changes preserve stored quadrature state; bloom-rate edits preserve
transport/random history. Contact and thump/FM envelope templates apply to the
next strike rather than restarting an ongoing gesture. The live-key classifier
is shared with UI help, and every advertised live parameter is regression-tested
against a freshly configured render after transitions settle.

The shared classifier also determines CLAP automation coverage. Recipe/key-derived
IDs are stable, independent of descriptor order; inactive recipes remain registered
but hidden. UI drags emit one begin/end host gesture, with values throughout.
Host playback updates visible controls without rebuilding them, and unrelated
automation is excluded from UI undo history. See [clap.md](clap.md).

Modal placement/allocation and packet texture use off-thread coefficient design
and a three-slot latest-update mailbox. The audio callback remaps the existing
state into that prepared layout without resetting the voice. See
[live modal editing](live-modal-editing.md) for identity, energy and transition
rules. Routing and observation delay still take the replacement lifecycle.
Offline spectrogram previews remain an independent, debounced worker and never
gate live control delivery.
