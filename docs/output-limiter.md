# Output protection

The native `output::Limiter` is an optional mono/stereo host-output stage. Link
`drumfoundry_output` from the future CLAP/standalone adapter. It is deliberately
not in `Voice`, the native raw-render C API, or Python fitting renders.

## Fixed settings and UI contract

- Enabled by default when preparing the output stage.
- Lookahead: **1 ms**, rounded to the nearest sample (48 at 48 kHz, 44 at 44.1 kHz).
- Ceiling: **−1 dB**, with the previous workbench's explicit **0.788 dB
  reconstruction reserve**. There is no makeup gain or normalization.
- Release time constant: **100 ms**. Peak hold: **30 ms**, to avoid bass flutter.
- Stereo channels share gain; neither balance nor stereo timing is altered.

The Visage shell has not been built yet. Its persistent output strip must show
**Limiter [on/off]**, a clearly labelled **gain reduction** meter in dB, and the
**actual latency**. Bypass must visibly say **Off — unprotected**. Peak reduction
and input overload are latched between UI polls, so short peaks cannot disappear
between redraws. Show nonfinite-input faults explicitly. Output peak is labelled
sample peak, not certified true peak. The ceiling and reconstruction reserve
belong in the limiter's visible explanation, not hidden gain compensation.

`Status()` returns this state. `ClearMeters()` clears interval peaks without
resetting gain or audio. The future host must publish coherent meter snapshots
to the UI; the UI must not read the audio-thread object concurrently.

## Delay and bypass

Peak reconstruction and gain smoothing share the same 1 ms audio delay. Delay
does not depend on callback size, and detector/envelope state persists across
callbacks. There is no attempt to predict unknown samples at block boundaries.

Bypass has **zero latency** and passes finite samples unchanged. Select it through
`Prepare(..., enabled)` while processing is stopped, not as an in-callback setter.
The CLAP shell must perform its safe restart/latency-notification sequence before
resuming. Do not drop/insert delayed samples mid-callback. Preparing/resetting
clears the delay; offline users must supply zero input to flush pending samples.

## Algorithm and limits

A four-phase windowed-sinc detector estimates intersample peaks. A sliding maximum
holds those peaks; bounded exponential release and two positive FIR averages
produce the gain. Their combined support fits inside the remaining lookahead
after detector latency. This follows the peak-hold/finite-smoothing construction
described by [Signalsmith Audio](https://signalsmith-audio.co.uk/writing/2022/limiter/),
with additional reconstruction and explicit headroom.

The reserve addresses quarter-phase sampling and FIR error. This is protective
output monitoring, **not a certified broadcast true-peak limiter**. Heavy limiting
can still distort audio; use sensible instrument/master levels. Nonfinite samples
become zero, with a latched fault indication, even in bypass.

Preparation allocates; processing, reset and meter reads do not. A monotonic deque
provides amortized constant-time peak hold. Short FIR gains use positive direct
sums to avoid cancellation after extreme overload.

## Tests

`./dev.ps1 test` covers exact delay/bypass, arbitrary buffer partitioning, hot
impulses on block boundaries, linked stereo, extreme/nonfinite input, meters,
20 Hz gain flutter, independent 16x reconstruction of high-frequency tones and
abrupt bursts, repeated kick/cymbal strikes, and allocation-free processing.
These are numerical checks; comparative listening in the native shell is still
required before claiming perceptual transparency.
