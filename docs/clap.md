# Native CLAP integration

The default build is a headless development shell. `./dev.ps1 ui` additionally
builds the Visage workbench, shared with the standalone; see
[native-ui.md](native-ui.md). Both remain development previews. It
links the same `drumfoundry_engine` used by Python; it contains no alternate DSP,
Python, Node, Wasm or web server. Windows uses the existing MinGW64 launcher.

## Build and package

`./dev.ps1 clap-test` enables `DRUMFOUNDRY_BUILD_CLAP`, builds the plugin and runs
the native suite plus a test host that dynamically loads the actual binary.
`./dev.ps1 clap-dist` creates a ZIP in `dist/clap` containing `plugins/`, the
engine preview, presets and licence notices. Normal `build`, `test` and `dist`
disable the CLAP target explicitly; no SDK download is needed for those targets.

Windows/Linux produce `build/native/TriggerFishDrumFoundry.clap`; macOS produces
a `.clap` bundle in the same directory. Install the file or entire macOS bundle
in your host's configured CLAP plugin directory. No automatic installation or
changes to existing host folders occur. These previews are not signed/notarized.

The MIT-licensed [official CLAP SDK](https://github.com/free-audio/clap/tree/195b42a004144fab0b3cf95e9c067187d15365b7)
is header-only, pinned to 1.2.10 and archive-hash checked in `cmake/clap.cmake`.
The project licence is the unchanged TriggerFish-VCV GPLv3 file; the SDK licence
is distributed separately, not substituted for it.

## Signal and control contract

MIDI note-on triggers the current voice at its sample offset. Velocity maps
linearly to strike strength (`velocity / 127`); all keys/channels currently strike
the same unpitched instrument. Note-off is ignored; CC120/123 resets the voice.
Repeated notes add strikes to the existing resonating state. Native CLAP note
dialect, pitched/polyphonic voices and MIDI mapping are not claimed yet.

The instrument selector embeds the existing kick, snare, hi-hat, crash, ride and
gong JSON fits at build time. Selecting an instrument restores its saved strike
hardness, implement, location, mute and gesture spread, including the initially
loaded kick. Master/protection remain unchanged. MIDI velocity supplies strength;
native pad gestures retain their unquantized floating-point velocity. The saved
seed remains in the instrument document. No fitting values are changed.

The host's generic editor exposes the selector, five strike controls, master
level, limiter, gain reduction, actual latency and the live-safe sound-design
controls described below. Kick ignores location as in
the core. Mute currently affects metallic voices only. Full JSON plus these
explicit performance overrides are stored in CLAP project state.
Gesture spread is appended at ID109; IDs100–108 remain unchanged. Earlier preview
states without ID109 restore its explicit value from their saved document.
The workbench's audition-velocity control is not a MIDI velocity multiplier:
it sets the fixed render gesture, and actual pad/MIDI notes update that gesture.

The mono voice feeds an explicit master gain (default -12 dB, 5 ms smoothing),
then the optional linked stereo limiter. Both output channels carry the same
mono signal; no widening or gain matching is inserted. The limiter is on by
default with 1 ms lookahead, or zero latency when bypassed. See
[output-limiter.md](output-limiter.md) for its ceiling and detector design.
Gain reduction is a positive dB readout held briefly for host polling. The
Visage workbench keeps protection, latency and reduction visible in its header.

Instrument changes, limiter bypass and loaded states request a host restart
when active. The old configuration keeps playing until the host stops audio.
Preparation then occurs during activation, with new latency reported through
CLAP's latency extension. A pending instrument selection wins over strike edits
made before that restart; edit its gesture after activation. This simple
headless lifecycle does not promise seamless structural live editing yet.

Live strike/master events are sample timed; processing never parses JSON,
prepares a new voice or allocates buffers. Saved/desired documents belong to the
main thread, current voices to the audio lifecycle, and cross-thread scalar
values are lock-free atomics. Invalid states are rejected transactionally. State
streams support partial reads/writes and a 1 MiB limit. No reference recordings
are bundled.

## Live design automation

Every parameter classified as live by `runtime/live_controls` is exposed from
the authoritative C++ metadata, in addition to the original performance controls:

- Final EQ/bypass, model and source gains.
- Metallic bloom/diffusion strength, concentration and energy sensitivity;
  modal T60 knot frequencies, times and enables.
- Contact and thump/FM templates, membrane damping and tension controls.
- Snare wire sensitivity, threshold, motion high-pass, engagement/release,
  brightness and noise/modal mixes (smoothed without restarting wire contact).

The registered list is fixed across presets. Each design ID is FNV-1a of
`recipe/key`, in the separate `0x40000000` namespace; collisions fail initialization.
Inactive recipe parameters are hidden, not removed or reassigned. Units, ranges
and defaults come from the same descriptors as JSON and the editor.

Events are applied at their sample offset without preparing/resetting a voice.
EQ and gains use 5 ms coefficient/gain smoothing, not added latency. Damping
and diffusion edits preserve stored energy; contact/envelope edits shape the
next strike. UI drags deliver values before release and emit one host begin/end
gesture. Host automation updates UI readouts without recreating the controls.
Saved project state includes current automated values and unconsumed UI edits.
Same-sample host parameter events (or one inactive `params.flush` batch) are
validated together: coupled T60 edits do not depend on event order. Invalid
curves are rejected as a unit without dropping unrelated EQ/gain changes.
Notes at that sample observe the completed parameter batch.

Saved design values use an atomic sequence-checked snapshot. Only the main-thread
reader retries; the audio writer never locks or waits. Pending UI edits are
included when their combination with the current host values is valid; otherwise
the accepted curve remains visible until the queue resolves the conflicting edit.

A structural preset awaiting restart has separate main-thread parameter storage.
The editor and saved project state show that validated pending preset, while host
automation continues to affect only the old sounding voice. When processing
stops, the pending values are accepted before preparing the replacement voice;
old-voice automation is not copied into the new preset. UI design edits resume
after activation. Coupled UI edit priorities are captured from the starting
document before sorting, never read from moving automation during the sort.

Mode geometry/allocation and packet texture now update during editor drags via
off-thread preparation and state-preserving block-boundary adoption. They are
**not** host-automatable yet: that needs a coalesced preparation worker, not heavy
work in the sample-timed scalar setter. This also applies to snare wire frequency
range, density and decay, and ring pitch/level. Routing and observation delay
still use replacement on release.
See [live modal editing](live-modal-editing.md). Factory selection and limiter enable
are exposed but not marked automatable; gain reduction and latency are read-only.
CLAP modulation and per-note modulation remain future work. Real-host recording
and playback smoke testing remains necessary in addition to the automated test host.

## Verification and next boundary

The host test loads the exported entry/factory and checks metadata, every preset,
event timing, invalid event rejection, finite protected output, stereo linking,
parameter text, independent instances, partial-stream state roundtrips, reset and
latency-changing restarts. CI runs it on Windows/MinGW, Linux, macOS ARM and Intel.
The supported rate range is 8–384 kHz. Repeated strikes are compared across
32/512-sample blocks at seven rates within that range. Inactive output readouts
clear to zero; they describe the active output, not stored patch parameters.

The official `clap-validator` 0.4.1 (`152b982`) was also run on the MinGW binary:
32 passed, 11 skipped (unsupported optional extensions/platform checks), and one
failure because its varying-rate test demands 768 kHz. That unsupported rate is
rejected explicitly; this is **not** an unrestricted validator pass. Parameter
conversion, event, state, transport and processing tests pass. Repeat with:

```powershell
clap-validator validate build/native/TriggerFishDrumFoundry.clap
```

Real DAW smoke testing remains necessary before treating this as a release.

The [console standalone](standalone.md) now hosts this same adapter with native
audio/MIDI devices. The Visage editor, shared native editing helpers and graphical
device management are implemented. Next: real-DAW GUI smoke tests and seamless
structural publication. The existing C API and Python
render/fitting paths remain raw and limiter-free.
