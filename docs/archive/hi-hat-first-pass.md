# Rejected first hi-hat fitting pass

**Status: first candidates rejected on audition.** Lower aggregate loss did not
establish a convincing timbral match. Do not use these runs as accepted
calibrations or as the basis of pedal interpolation yet.

First fit closed, half-open and open separately. Pedal interpolation and foot-chick
synthesis are deferred until these sounds have been auditioned. No new runtime
topology, MIDI handling or factory presets are introduced by this experiment.

## Dataset and comparisons

Use one local 14-inch reference set, four recordings per articulation. The
catalog supplies provisional strengths: these are **not verified recorded MIDI
velocities**. Their energy ordering is consistent, but their amplitude ratios
are not a measured velocity law. Do not silently estimate new strengths or
normalize individual recordings to make that uncertainty disappear.

Each state uses one parameter vector for all four strengths. Source audio is
averaged to mono at its native 44.1 kHz, aligned using catalog onsets and given
the same explicit +8 dB reference gain. No per-layer playback gain matching,
output EQ, limiter or velocity remapping is added. Static strike constraint
is zero: state damping is expressed by the visible T60 curve, not a second mute.

## Restricted parameterization

All three start with the same 24-handle stretched harmonic series: 260 Hz base,
stretch 0.22, three protected low harmonics. This uses the modal editor's
protected-core law, materialized as ordinary JSON parameters. Handles have even
prominence and allocation; no individual frequencies, gains or local decay
multipliers are optimized. The global T60 curve has just its two endpoints.

Bounded Powell stages fit level/damping/excitation tilt and velocity brightness;
contact/bloom; then packet texture; then revisit level/damping. Log search
coordinates are explicit for positive time, width and frequency controls.
Parameters not named in the stage remain fixed. The full bounds and every trial
are stored with the source fit and native-library hash.

After the state fits, `refine_hat_series` tests a small **shared** base/stretch
grid against all twelve recordings: bases 220–300 Hz, stretches 0.14–0.26,
24 handles and three protected harmonics. It retains the unchanged structure
as a candidate and rejects a shared tuning change if any state's RMS error
increases by more than 0.25 dB. No isolated ridge adjustment is involved.

The shared-series pass also makes observation headroom explicit: all active bars
move from -12 to 0 dB, direct observation gain rises by the same 12 dB, and model
level initially falls by 12 dB. A native render test verifies this is an equivalent
representation, not extra excitation or a different sound. It then refits the
single model level across all layers within -40–0 dB. This avoids a poor fit
caused merely by the first pass hitting the model-level ceiling. Every resulting
value is an ordinary, visible parameter; no automatic playback compensation exists.
Then `polish_hat_states` revisits **level and both T60 endpoints together** with
the restored headroom, while freezing geometry, texture, excitation and velocity
response. Refitting gain alone cannot repair a decay compromise made at a gain
limit. This is still a two-point decay curve, not another envelope or local loss.

The first-pass loss pools **linear power** into ERB triangular bands before
taking logs: 512/2048-sample Hann STFTs, 24/48 bands, 75% overlap. This compares
spectral density rather than assigning identities to unresolved upper ridges.
Six time regions (0–30, 30–100, 100–300, 300–700, 700–1500, 1500–3000 ms)
receive equal weight. The floor is reference-fixed at 50 dB below each
resolution's peak. Layers receive equal weight, not loudness-dependent weight.
It is an engineering loss, **not** a perceptual acceptance criterion.

### Current priority: timbre, not the library's unknown velocity curve

The final `--timbre` pass ignores **one constant overall level offset per layer
inside the comparison only**. It scales a temporary analysis copy of the candidate
to the reference's total RMS, then measures spectral/time differences with the
same reference-fixed floor. This is not envelope compression: attack/body/tail
ratios, bandwise decay and spectral balance remain scored. The offset is reported.
No source file, render, playback gain or MIDI event is normalized/remapped.

That pass freezes model level and geometry, fitting only the two T60 endpoints,
excitation tilt, turbulence slope, phase bandwidth and velocity brightness.
Absolute level errors remain diagnostics, not evidence that the voice's timbre
or velocity response is wrong: the source library's velocity curve is unknown.
Compression/velocity-to-level mapping is a separate later decision.

## Run and audition

```powershell
python -m drumfoundry.refinement.fit_hat_states --source local-hat.fit.json `
  --root D:/References --output build/hat-states-01 --budget 110
```

`--publish` optionally names a **new** folder beneath the configured user-preset
directory. Existing files are never replaced. Each state has four audition
entries sharing exactly the same sound parameters; only the reference and strike
defaults differ. Reports include held-out-seed errors, raw peaks, energy error,
reference hashes and exact reload checks. Raw comparison WAVs have no limiter;
audition through the native workbench master/limiter. No separate player/server.

To inspect completed results and publish only after review:

```powershell
python -m drumfoundry.refinement.inspect_hat_states --run build/hat-states-01 `
  --root D:/References --output build/hat-inspection-01 --perceptual
python -m drumfoundry.refinement.publish_hat_audition build/hat-states-01 D:/UserPresets/Hi-hat-trials
```

Optional shared-series pass (fresh output, preserving the first run):

```powershell
python -m drumfoundry.refinement.refine_hat_series --run build/hat-states-01 `
  --root D:/References --output build/hat-states-series-01
python -m drumfoundry.refinement.polish_hat_states --run build/hat-states-series-01 `
  --root D:/References --output build/hat-states-decay-01 --budget 100
python -m drumfoundry.refinement.polish_hat_states --run build/hat-states-decay-01 `
  --root D:/References --output build/hat-states-timbre-01 --budget 90 --timbre
```

Inspection writes fixed-scale Plotly data and checks repeated hits. Optional
`--png` needs Kaleido (development-only); `--perceptual` also reports the retained
reference-floor multiresolution Mel loss on the medium layer. It does not choose
a different winner. Publication creates three main presets and a separate
"Velocity comparisons" subtree; it validates that layer views share parameters
and all three states share their modal frequencies. Reopen the native Preset
menu to discover the new files; no rebuild or application restart is needed.

The unchanged old model is measured with the same fixed test gestures. Candidate
selection uses seed 1944; seed 73519 is reserved for audit/export. All four
velocity layers are training data, not held-out validation recordings. Additional
takes with known MIDI velocities would strengthen validation.

Known-answer tests cover identity, wrong gain/decay, narrowband-versus-noise
differences, equal layer weighting and preservation of the restricted series.
Review band decay, spectrum, texture, velocity response and repeated hits before
accepting a candidate. An improved aggregate score can still hide a bad layer.

### Assessment failure found in the first audition

The fixed three-second objective includes equally weighted, nearly silent
regions. In the medium closed layer, its last two regions score zero while
0–30 ms still has about 8.8 dB spectral error. Reporting only the aggregate
dilutes this audible mismatch. Report each active region separately; silent
regions must still penalize an unwanted tail, but must not earn matching credit
that makes an inaccurate attack look acceptable.

The shared tuning grid was also very narrow, and the final timbre pass froze
contact and much of the texture after an absolute-level fit. These restrictions
need reassessment before another search; matching durations alone is insufficient.
Retain structured series rather than freely painting ridges, but do not interpret
"somewhat consistent" as requiring identical modal frequencies across states.

Saved presets can be checked through the plugin's actual edit/restart/pad path,
reusing one plugin instance, without opening audio devices:

```powershell
./dev.ps1 ui-test
./build/native/tests/clap_editor_events_tests --audit-presets closed.fit.json half-open.fit.json open.fit.json
```

This checks native-render parity and reports remaining energy after 300 ms. It
tests loading/playback correctness, **not** perceptual quality or the user's
particular running application instance.

### Open-state noise diagnosis

One-factor native renders isolate excessive phase blur: muting direct contact
or turning diffusion off leaves the 100–1100 ms noise-like ridge contrast
essentially unchanged. Zero phase blur restores very narrow lines, overshooting
the reference in the other direction. This is not evidence for replacing the
resonator engine or adding another noise generator.

The published open state has noisiness about 3.53, spread about 2.57 ERB and
phase blur about 0.24 ERB. In the existing engine, effective spread multiplies
noisiness and effective phase bandwidth multiplies its **square**. At this
noisiness almost all initial packet energy goes to satellites, not the centre.
The nominally modest blur therefore produces broadly overlapping noisy modes.

As a descriptive check, locally whitened spectral flatness in 1.5–3 / 3–6 /
6–12 kHz is approximately -6.8/-6.5/-4.4 dB in the reference and
-2.6/-2.5/-2.4 dB in the published fit. Noisiness 0.6 with blur 0.01 gives
-8.0/-6.7/-5.1 dB, demonstrating a less smeared region of the existing control
surface. This is an ablation, **not** a fitted or approved replacement: its
tuning, band balance, temporal texture and velocity response still need checking.
The scalar contrast statistic alone cannot establish perceptual similarity.

## Later pedal convention

Toontrack uses CC4, with low values open and high values closed/tight. Follow that
direction, with an inverse mapping for an "openness" slider. This is documented
in [Toontrack's hi-hat controller explanation](https://www.toontrack.com/forums/topic/need-laymans-explanation-of-how-to-control-hi-hat-open-close/).
A later continuous pedal must damp existing energy without resetting the voice;
only sufficiently fast closure should produce diffuse chick excitation.
