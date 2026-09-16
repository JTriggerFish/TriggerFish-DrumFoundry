# Saved-fit refinement

These are offline development tools, not runtime dependencies. All synthesis
uses the native C++ voice through `drumfoundry.Renderer`. There is no server,
Node transport, alternate Python synthesizer or automatic preset publication.

For separately auditionable hi-hat states fitted jointly across velocity layers,
see [static hi-hat fitting](hi-hat-fitting.md). That experiment uses a shared
stretched series and does not add pedal interpolation yet.

## Inputs and invariants

Start from one locally saved fit and its selected WAV. Keep the saved strike
velocity, implement, hardness, location, routing and output levels fixed unless
the experiment explicitly tests those controls. Never average seed waveforms.

Reference channel, onset and gain come from the attachment, or explicit CLI
overrides. Record raw-file SHA256, aligned PCM SHA256 and resampling method.
Python uses SciPy polyphase resampling, not the UI's libsamplerate sinc converter;
use the reference's native sample rate when investigating small differences.
There is no automatic normalization, gain matching, limiter or EQ insertion.
Raw float WAVs may exceed 0 dBFS; audition through the workbench's master/limiter.

## Running a stage

Install the optional development environment and build with `dev.ps1`.
Metal's perceptual term additionally needs `uv sync --group dev --group perceptual-fit`.
Use the virtual environment's Python; the examples assume it is activated.

```powershell
./dev.ps1 build
python -m drumfoundry.refinement --fit my-crash.fit.json --reference D:/Audio/References/crash.wav `
  --library-root D:/Audio/References --output build/crash-trial-01 `
  --profile metal --stage locked-texture --budget 180 --plots

python -m drumfoundry.refinement.audit build/crash-trial-01 `
  --reference D:/Audio/References/crash.wav --output build/crash-audit-01
```

Omit `--library-root` only when the source fit already has the correct relative
`reference.libraryPath` and a matching file hash. The selected file must lie inside a supplied root.
Outputs require a fresh/empty directory. Source and candidate documents are
frozen with hashes; the audit rejects changed documents or reference audio.

Load `candidate.fit.json` in the native workbench to inspect and listen. The
candidate carries its reference attachment. Nothing changes factory or user
presets automatically. Optional Plotly JSON contains fixed-level spectra,
band envelopes and reference-scaled STFT/difference plots, not another player.

## What gets fitted

| Profile / stage | Coordinates and method |
|---|---|
| `metal / locked-texture` (default) | Two endpoint T60s, whole-series pitch and smooth upper stretch; bounded Powell. Prominence, packet texture and transfer stay fixed. |
| `metal / shared-series` | Explicit texture alternatives followed by nine shared tuning, damping, bloom, excitation and broad prominence coordinates; no individual-ridge search. |
| `metal / upper-balance` | Broad upper prominence slope; no output EQ. |
| `metal / low-decay` | Three low-T60 multipliers. |
| `metal / sparse-decay` | Transfer-control trials first, then one shared interior damping knot. Requires a two-endpoint starting curve. |
| `metal / fine-decay` | One sparse damping knot only, as a final-stage experiment. |
| `gong-onset / gong-grid` | Nine explicit excitation-tilt / upper-T60 pairs; preserves the existing modal series. |
| Any profile / `coordinates` | Only named, bounded continuous native controls from `--bounds file.json`. |
| Any profile / `audit-only` | No edits; render and measure the existing fit. |

The metal search ranges are starting-fit-specific, not universal defaults.
The unchanged input always remains a candidate, even if outside a stage's
narrow search box. Every evaluated parameter vector and component score is saved.
Two endpoint damping is preferred; extra knots require an explicitly chosen
final stage. No automatic per-mode decay, mode insertion or isolated-ridge edits.

For explicit coordinates, bounds use `{ "parameter_key": [minimum, maximum] }`.
Bounds must contain the initial value and remain within the native descriptor.
Powell works on normalized coordinates. Kick, short-drum and gong-onset also
support `--solver least-squares`: bounded TRF with an explicit absolute-step
Jacobian. `--difference-step 0.005` means 0.5% of each coordinate's range, even
at zero. Initial sensitivities are reported; there is no hidden pruning threshold.

`--budget` limits Powell evaluations or TRF main residual evaluations. Finite
difference probes and fixed-grid screening are additional evaluations; the full
trace records them. Fixed grids do not use the budget, except zero means no search.
Phase realizations are held fixed within a stage; subsequent audits use other seeds.

## Objectives and acceptance

Metal uses six seconds and the retained reference-floor V3 objective:

$$L=L_{Mel}+0.3L_{attackMel}+0.05L_{bloom}+0.15L_{texture}+0.15L_{decayShape}.$$

This combines reference-fixed multiresolution Mel STFT, early attack, bandwise
bloom trajectory, modal texture and band-decay shape. Only quiet, flat terminal
bands receive the documented recording-floor mask in the envelope comparison.
The exact windows, band edges, floors and weights are stored in `report.json`.
Selection averages scalar loss over two training seeds.

Kick uses the retained `RidgeBalanceLoss`: reference-relative spectral/ridge
balance and band-envelope terms. Short drums can use `ShortDrumLoss` instead.
Both operate over 1.2 seconds. Gong onset uses three seconds, causal band filters
and explicit early/late regions: fit 80–800, 5000–9000 and 9000–14000 Hz; report
2500–5000 Hz separately without forcing away the audible midrange gap. These
vector objectives combine separate seed residuals by RMS norm, not audio averaging.

The saved report contains the objective specification, reference conditioning,
native library hash, source/candidate hashes, trace and held-out seed scores.
The independent audit reopens both documents and rerenders them with the current
engine, recording its hash separately. It verifies exact reload, reports held-out
metrics and checks three velocities plus five-hit sequences at two intervals.
For metal it also compares band-modulation and ridge-contrast diagnostics against
the reference. Velocity probes are robustness checks, not fits to other layers.

**Lower loss is not listening approval.** Inspect pitch, attack, bloom, band decay,
texture and raw peaks separately. Listen to the candidate against its actual
reference in the workbench before replacing a local fit. These tools preserve
the latest method; they do not claim the existing calibrations are accurate.

## Tests and retained tools

`./dev.ps1 python-test` checks reference identity/conditioning, untouched gesture
and routing, exact reload, known-answer decay/ridge/modulation losses, bounded
search and overwrite rejection. `./dev.ps1 perceptual-test` explicitly exercises
auraloss, WaveSpin and the structured metal objective. Ordinary builds do not
install these dependencies. `./dev.ps1 benchmark-percussion` builds the retained
DSP component and block-deadline benchmark as an opt-in target.

Historical server scripts, automatic calibration publication, per-ridge search
recipes and obsolete linear observation surrogates are not active dependencies.
Shared numerical losses remain usable with explicit native fitting coordinates.

## Upper-frequency coverage

The metallic stretched-series helper supports the native 20 kHz modal limit.
Temporal-metal v4 and tonal-layer v3 score spectrum and ridge texture through
20 kHz (or 0.48 times the sample rate when lower), including explicit upper-band
temporal diagnostics. Earlier objective scores are not directly comparable:
re-evaluate both baseline and candidate with the same version. Existing saved
fits are not modified by this change. Known-answer tests check that removing
upper-band energy worsens the loss, alongside identity and gain invariance.
