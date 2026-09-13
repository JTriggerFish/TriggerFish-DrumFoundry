# Standalone device-test application

The standalone can be built as a **console audio/MIDI application** or with the
shared Visage editor. Both use the actual CLAP adapter with real device callbacks.
The optional `./dev.ps1 ui` build adds the native workbench and
audio/MIDI settings panel; see [native-ui.md](native-ui.md). That build opens
the window with no arguments, while console commands remain available.
The plugin and executable link the same compiled adapter objects; no synthesis,
preset or limiter implementation is duplicated. Python is not involved.

## Build and run

```powershell
./dev.ps1 standalone-test
./dev.ps1 standalone-dist
./build/native/TriggerFishDrumFoundry.exe --midi-list
./build/native/TriggerFishDrumFoundry.exe --api asio --device "MOTU M Series" --list
./build/native/TriggerFishDrumFoundry.exe --api asio --device "MOTU M Series" --buffer 128 --midi all
```

On macOS/Linux omit `.exe`. macOS uses `--api core`; Linux uses `--api alsa`.
Windows also supports `--api wasapi`. `--list` lists device names and supported
rates for the selected backend; supply a matching `--device` substring to play.
Ambiguous names, unavailable backends and unsupported rates are errors, not
silent fallbacks. Close other applications using an exclusive ASIO device first.

The default rate is 48 kHz and requested buffer is 256. The driver may negotiate
a different buffer: both requested and actual sizes are printed. `--smoke`
tests all six instruments without opening audio or MIDI hardware. `--test-seconds 5`
opens the selected device for five seconds, prints diagnostics and exits. This
does not generate notes unless `--audition` is added or MIDI notes arrive. Use
`--midi none` for a silent stream test. Hardware-free checks run in CI; they are
not evidence that a physical interface was exercised.

## Console controls

`hit 100` triggers a strike, `panic` resets the voice, `status` prints device and
protection diagnostics, `quit` releases audio and MIDI. Other commands:

- `preset kick|snare|hihat|crash|ride|gong`
- `master -18`, `hardness 0.5`, `implement 0.5`, `location 0.5`, `mute 0`
- `limiter on|off`, `buffer 128`, `rate 48000`

Limiter defaults on, with the same fixed 1 ms lookahead and explicit -12 dB master
as the CLAP shell. There is no second limiter, normalization or sample-rate
conversion in the host. The driver-reported latency and plugin lookahead are
shown separately; neither is presented as a measured end-to-end keyboard latency.
The console provides a `status` readout; the Visage build adds persistent live meters.

Structural changes stop/join device processing before preparing the plugin again.
This resets resonating state and discards pending notes, preserving queued
parameter edits before applying the new selection. Live gesture/master controls are queued into the
next audio block. The native GUI remembers device choices as described below;
console device tests do not change them or system device defaults.
Errors print in the console; callback fault, overflow and
xrun counters are included in diagnostics. Device tests return a failing exit
status for xruns, dropped events, device/process errors or no callbacks. Timed
tests use monotonic deadlines, avoiding cumulative sleep rounding.

## MIDI and audio-thread boundary

`--midi all` opens available inputs (up to 32), `none` disables them, or an exact
name selects one input. Console abbreviations must match one port unambiguously.
Opening failures identify the port; other successful inputs remain open in `all`
mode. In the GUI a MIDI failure leaves audio running, so manual strikes and
reference playback remain usable. Windows diagnostics retain the WinMM error
number and OS explanation rather than replacing them with a generic message.
`--midi-check --midi "SL GRAND 1"` opens/closes only that input, without audio,
and returns failure if it cannot open. This is a driver diagnostic, not a test
of MIDI note delivery. All note-on
keys/channels strike the selected instrument with linear velocity; CC120/123
reset it. No pitch mapping, sustain or hi-hat pedal-CC mapping is implemented yet.
The MIDI menu rescans when opened; Apply retries the current selection after a
device reconnect. Port identities currently use backend names, not persistent
hardware IDs; if the OS renames/reorders a device, select its new name.

Each input has its own bounded SPSC event queue, with a separate control queue.
RtMidi callbacks copy three-byte messages into those queues. The audio callback
collects a bounded number round-robin into fixed CLAP event arrays, so a busy
port cannot starve another input, and interleaves the stereo
output. It does not allocate, lock, log, enumerate devices or parse JSON.
Queue saturation is counted; excess queued events wait for the next block.
Live MIDI arrives at the next callback boundary (up to one buffer of scheduling
quantization); this version does not claim sample-accurate external MIDI timestamps.
The CLAP adapter itself retains sample-offset event processing.

## Dependency choices

[clap-wrapper](https://github.com/free-audio/clap-wrapper/blob/1cca996e96f29ab2be7ae9f8cfe532bbc92e1dd6/cmake/wrap_standalone.cmake)
has a standalone host, but its Windows GUI target selects MSVC/Clang, not MinGW
GCC. Its non-GUI fallback also does not provide our required parameter/device
surface. This small host keeps the existing MinGW toolchain and leaves a clear
boundary for the later Visage interface.

RtAudio and RtMidi are commit/hash pinned, with only selected backend sources
compiled. Windows has ASIO/WASAPI and WinMM MIDI, macOS CoreAudio/CoreMIDI, Linux
ALSA audio/MIDI. Normal engine/CLAP-only builds do not fetch these dependencies.

For ASIO, the build uses the explicitly GPLv3-available 2025 SDK from the pinned
`audiosdk/asio` mirror, rather than the older SDK copy bundled inside RtAudio.
The SDK's GPLv3 option is described on [Steinberg's site](https://www.steinberg.net/developers/asiosdk-open/).
The repository's original GPLv3 license is unchanged. Separate library/SDK
notices are included in the package; no driver or vendor control panel is shipped.

A small checked source-generation patch restricts RtAudio ASIO probing to the
requested driver name, before initialization. This prevents an explicit MOTU
selection from initializing unrelated registered drivers and potentially showing
their missing-hardware dialogs. `--list --api asio` without a name intentionally
probes all drivers; prefer a `--device` filter. The downloaded source is untouched,
and the patch lives in `cmake/rtaudio-probe.cmake` for review with dependency updates.

`cmake/asio-compat.cmake` also retains RtAudio's upstream `ASIOExit` null-owner
guard when compiling the newer GPL SDK. RtAudio owns its own `AsioDrivers` and
explicitly releases the driver after `ASIOExit`; the SDK's optional global
`asioDrivers` pointer stays null. Omitting this guard caused a hardware-probe
crash and is covered by the hardware-free `asio_cleanup_tests` regression.

## Local hardware verification — 2026-09-13

MOTU M Series ASIO at 48 kHz, with Bitwig closed:

| Test | Actual buffer | Driver-reported latency | Result |
|---|---:|---:|---|
| Five-second silent stream | 256 samples | 342 samples | 957 callbacks, zero xruns/errors |
| Five-second silent stream | 128 samples | 214 samples | 1916 callbacks, zero xruns/errors |
| Five-second kick audition, ten strikes | 128 samples | 214 samples | 1914 callbacks, zero xruns/errors |
| Live SL GRAND MIDI, gong (60-second requested test) | 128 samples | 214 samples | 24471 callbacks, 82 MIDI messages, zero xruns/errors/dropped events |

The limiter adds 48 samples (1 ms). These are driver/plugin reports, not a
loopback measurement of end-to-end MIDI-to-speaker latency. The user also
confirmed successful live playing from SL GRAND into the gong at 128 samples.
These measurements precede the deadline/restart-queue review fixes. The original
relative sleeps ran slightly longer than requested; callback counts are retained
as recorded, not presented as precise wall-clock timing.

## Native interface

Visage supplies the settings/device chooser, strike surface, persistent limiter
meters, modal/T60 editors, snapshots and reference analysis. It calls this host
layer, while CLAP embeds the same workbench. There is no browser server or second
DSP renderer. See [native-ui.md](native-ui.md) for interaction and testing details.

### Remembered audio/MIDI settings

The GUI saves the requested audio API, output device, MIDI input, sample rate and
buffer on **Apply & start**, including when a disconnected device needs retrying.
On the next launch these selections return, but devices remain closed until
Apply: restoring preferences must not seize an exclusive ASIO/MIDI device from
a DAW. Explicit GUI command-line device options override remembered fields;
`--device` also requests immediate start. Smoke/console tests neither restore
nor write preferences. Release device does not erase them.

Settings use a separate `audio-midi.json` beside the application-data `fits`
directory (Windows: `%LOCALAPPDATA%/TriggerFish/DrumFoundry/audio-midi.json`).
Writes replace atomically; invalid/unreadable files produce a visible warning
and fall back to startup defaults. They never alter fit JSON, DSP levels,
limiter settings, system defaults or the selected instrument.

During the 2026-09-13 MIDI investigation, opening both SL GRAND WinMM ports
directly, outside DrumFoundry and without ASIO, returned **MMRESULT 7**
(`MMSYSERR_NOMEM`) while approximately 18 GB RAM was free. This reproduces a
device/backend failure but does not establish its cause; reconnect/retry is
needed to verify recovery. No driver reset or unrelated process termination
was performed. App-side handling is separately covered by injected port-failure
tests, exact-name selection tests and settings persistence tests.
