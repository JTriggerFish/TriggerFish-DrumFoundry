# Hi-hat calibration

Current scope: **open hi-hat**, fitted across four layers of one local 14-inch
reference set. Closed/half-open fitting and pedal interpolation remain separate.
References, renders and calibration presets stay outside Git.

## What is fitted

Python renders the actual native C++ voice. No substitute synthesis, output EQ,
compression, limiter or per-layer playback normalization enters the fit.
The source library's MIDI velocity law is unknown: catalog strengths remain
fixed, and one constant level offset per layer is ignored **in analysis only**.

- Begin with a stretched harmonic series, not independent upper ridge positions.
- Compare several bases/series densities; keep the upper handles evenly related.
- Fit excitation colour, contact, diffusion, packet spread/noisiness and blur.
- Use two T60 endpoints first. Test one interior knot only after inspecting
  the remaining band-decay mismatch. Never fit local T60 multipliers here.
- Allow limited refinement of clearly identified low packets' pitch, coherence and prominence.
  Upper prominence remains a common smooth tilt, not independently fitted bars.
- Quiet lower harmonics must remain available; excluding them can remove real
  low-frequency content rather than simplify the problem.

All resulting values are ordinary serialized controls. Series-generation and
relative-prominence coordinates are materialized into the existing modal editor.
No fitted coefficient is added secretly to the engine.

## Objective and inspection

The rejected first recalibration used `TonalLayerLoss`, combining:

1. Active-region ERB spectral/time error. Matching silence does not dilute an
   inaccurate attack; unwanted energy in quiet regions still incurs error.
2. Log-frequency spectral shape in three time regions. Weights combine 30%
   uniform coverage with 70% reference square-root band energy, so a prominent
   low resonance is not outvoted by numerous quiet bins.
3. Locally whitened spectral contrast in five bands and two time windows.
   This distinguishes narrow resonant texture from a noise wash without
   assigning identities to individual upper-frequency peaks.

For component errors $E$, $S$, and $C$, the search score is

$$L=\sqrt{(E^2+S^2+4C^2)/6}.$$

It improved line/noise contrast without establishing a correct initial tone or
bloom. It is retained for comparison, not as the current acceptance method.

The replacement `TemporalMetalLoss` reuses the gong's **causal band-analysis**
approach at the hat's shorter timescale:

- Fourth-order band filters and disjoint 10 ms power windows; no centred FFT
  smearing of energy before an onset in the temporal comparison.
- Separate onset (0–100 ms), bloom (100–350 ms), decay (350–1500 ms) and terminal
  tail (1500–3000 ms) errors. The last region still scores unwanted energy when
  the reference is quiet.
- Explicit within-band rise relative to the first 30 ms. Premature high energy
  cannot substitute for a later rise with the same averaged spectrum.
- Early low-body magnitude spectral convergence, alongside log-spectrum shape.
  This prevents matching numerous quiet valleys at the expense of the audible
  initial tonal ridge. Upper ridges still have no individually fitted identities.
- A subordinate texture comparison. All weights and measurement regions are
  serialized in the report; the aggregate `error_db` is an engineering ranking,
  not a literal perceptual error in dB.

Known-answer tests check a tone followed by high-frequency sizzle, premature
upper energy, wrong decay, lingering terminal energy and constant gain changes.
No test result establishes a good fit to the real recordings.

This is an engineering objective, **not perceptual approval**. Reports contain
its FFTs, windows, frequency bands, floors and weights. Inspect individual
components/layers, fixed-colour spectrograms, signed differences, spectra and
band envelopes. Do not accept a smaller aggregate score while a salient
resonance or the audible decay remains plainly wrong.

The diagnostic line/noise statistic alone is also insufficient: zero blur can
overshoot into sparse, unnaturally sharp resonances.

## Execution and artifacts

`fit_open_hat` searches structured starting families. `refine_open_hat` continues
saved checkpoints for texture/body/decay. `fit_hat_body_ridge` restricts low-end
refinement to one dominant packet. `inspect_open_checkpoint` produces local
Plotly figures/optional PNGs, not a separate audition website.

The temporal redo uses `fit_temporal_hat`, `hat_gong_starts` and
`refine_hat_texture`. Coarse trials explore active diffusion regimes first:
the rejected preset's rate control had nearly zero influence at its fixed
energy/concentration settings. Bounded least squares then uses explicit 1%-range
finite differences and records sensitivities. A single smooth observation tilt
and the prominent low packets' prominence/coherence may change; the upper series
is not freely painted. No DSP topology or runtime parameter is added.

Each evaluation uses all four layers. `LayerFit(..., workers=N)` uses the same
evaluation/seed/logging path at every worker count. Concurrent tasks own separate
native voices; tests verify serial/parallel parity with per-layer training seeds.
Each stage
records full parameter vectors, bounds and layer errors. Exported layer views
share synthesis parameters and differ only in saved strike/reference metadata.

Initial searches use seed 1944; final texture/decay work uses 1944 and 7823,
concatenating residuals, never averaging waveforms. Audit seed 73519 is not
optimized, but it has been inspected during selection and is not an unseen
test case. Additional independent seeds are needed for the final robustness audit.
Export checks exact saved
JSON reload/render reproduction. Repeated-hit and plugin-path tests remain
required. These seeds do not make the four training recordings held-out data.

A single saved output gain may be chosen from the complete layer grid, with
headroom checked separately. Equivalent observation restaging scales body and
contact output together; it must not change excitation or stored energy.
There is no runtime gain matching. Audition uses the native workbench limiter.

Build/test via `dev.ps1` (MinGW on Windows). Python, Plotly and optional Kaleido
are offline development dependencies only. For hardware-free saved-preset tests:

```powershell
./dev.ps1 ui-test
./build/native/tests/clap_editor_events_tests --audit-presets open.fit.json
```

## Why the first pass failed

The rejected preset combined high noisiness with excessive phase blur. Effective
blur bandwidth scales with noisiness squared, so nominally moderate blur smeared
the modal structure. Broad-band matching tolerated this; silent late regions
also diluted attack errors. See the [first-pass record](archive/hi-hat-first-pass.md).

Pedal work remains deferred until static sounds are auditioned. The intended
convention is CC4, low=open and high=closed, following
[Toontrack's controller convention](https://www.toontrack.com/forums/topic/need-laymans-explanation-of-how-to-control-hi-hat-open-close/).

## Temporal redo outcome — partial, not accepted

The current audition retains the dense structured series, protects low partials
near 470 and 1410 Hz, and uses intermediate ridge movement with modest blur.
A sparse 470-Hz-rooted series lost too much surrounding spectrum; merely making
the low packet louder also failed to solve its definition and temporal balance.
The saved candidate was selected after inspecting fixed-scale spectrograms,
not because it had the lowest aggregate search score.

Across four layers and three audit realizations, separate RMS diagnostic errors
changed as follows: onset 4.92→3.75, bloom region 4.32→3.52, decay 4.50→3.36,
terminal tail 2.95→2.49, and low-body magnitude comparison 7.60→5.82.
**The within-band rise error worsened slightly, 4.15→4.30.** Texture error also
increased, 1.27→1.69. These numbers do not establish audible similarity.
The initial tone remains too weak against the upper sound, and the transition
into sizzle is still insufficiently separated. Do not label this calibration
complete or use passing implementation tests as evidence that it sounds right.

Only the local open-hat audition and its four velocity views were replaced;
previous files were archived. Closed/half-open, the engine, factory presets,
normal velocity mapping and the output limiter were not changed.

## User-authored open-hat refinement: late sizzle

This subsequent experiment starts from the user's edited open-hat document,
not the earlier automatic calibration. It targets its saved strike and reference
at 48 kHz without changing reference gain, model level, contact parameters,
modal frequencies or adding EQ. It is a **local audition candidate**, not a new
factory preset or a completed multi-velocity calibration.

The initial diagnosis separated high-frequency level from its fluctuations:
the saved sound overemphasized roughly 6–12 kHz, underplayed late 16–20 kHz,
and had a smoother high-band envelope than the recording. More phase blur did
not recover those fluctuations. Paired rings split the central oscillators;
beating doublets instead split the sidebands carrying most of this high end.

The search used discrete sideband-layout/density/movement trials followed by
bounded least squares over excitation tilt, diffusion strength/concentration,
two T60 endpoints, packet spread, and two broad prominence shapes. The prominence
shapes are a log-frequency Gaussian centred at 7.5 kHz (0.65-octave sigma) and a
smooth upper shelf centred at 12 kHz. Their effects are written into ordinary,
visible modal levels; they are not hidden runtime controls or independently
fitted bars. Modal frequencies stay exactly as the user placed them.

The residual compares causal fourth-order band-filter energies in six regions
(0–30, 30–100, 100–250, 250–500, 500–1000 and 1000–1500 ms). Nine bands span
100 Hz–20 kHz. Late upper-band errors receive 1.5× weight; lower-band departures
over 1 dB from the user's first-second response receive an additional penalty.
Native evaluations use two fixed training realizations and explicit finite
differences of 2% of each search range. Three unused realizations, exact saved
document reloads, and repeated hits at four strengths are checked afterwards.

Low-density/no-motion trials improved a fluctuation statistic but visibly
collapsed into sparse stationary lines: **rejected despite their numerical
improvement**. Shared bounded ridge movement retained the paired beating while
restoring spectral motion. The final candidate uses beating doublets, density
0.2, beat depth 1, beat rate 8 Hz (with the existing frequency tilt), ridge
movement 2, packet sharing 1 and no unbounded phase blur. The two T60 endpoints
are about 2.40/2.91 seconds; diffusion strength/concentration are about
1.33/0.66. This is still an imperfect approximation, especially in low/mid
decay detail and the very end of the recorded tail.

`refinement.modulation.band_fluctuation_db` is the reusable supporting
diagnostic: causal band filtering, 5 ms mean-power windows, then a 50 ms Gaussian
trend removed from the dB envelope. In the 12–20 kHz, 150–1300 ms region, the
saved sound measured about 0.67 dB RMS, the candidate 2.02, and the reference
2.17. This is not a perceptual loss or proof of similarity. Synthetic tests
check gain invariance, rejection of smooth exponential decay, and sensitivity
to amplitude modulation. Fixed-scale spectrogram inspection remains necessary.

Private run artifacts are under `build/hat-sizzle-refinement-01` through `-05`;
the final reload/seed/restrike audit and publication receipt are under
`build/hat-sizzle-audition-final`. Earlier experiment candidates are not exposed
in the preset menu. The published local preset is **Hi-hat open — late sizzle
candidate**; the user's source document is preserved.
