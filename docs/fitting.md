# Native rendering and fitting

Python is optional, offline tooling. `drumfoundry.Renderer` calls the native C ABI
directly; C++ reads and validates the saved document, maps parameters and runs
the same voice that the future CLAP adapter will use. There is no JS renderer.

`triggerfish_percussion` retains selected existing numerical modules: STFT,
power envelopes, band-decay shape, short-drum and metallic losses, modal texture,
and optional perceptual losses. These are engineering diagnostics, not proof
that a fit sounds right. Optional perceptual packages are a separate uv group:

```powershell
uv sync --locked --group dev --group perceptual-fit
./dev.ps1 perceptual-test
```

This includes auraloss and the same commit-pinned WaveSpin implementation used
by the original project. The opt-in tests check both backends execute and prefer
identical audio over a changed signal. They deliberately are not silently skipped
when dependencies are missing, and are not required by normal builds/tests.

```python
from pathlib import Path
from drumfoundry import Renderer
from drumfoundry.fitting import fit_parameters
from triggerfish_percussion.short_drum_fit_loss import ShortDrumLoss

# reference is already mono PCM at the render rate, with explicit fixed gain
# and onset alignment; never normalize the model independently per candidate.
with Renderer(Path("presets/factory/kick.fit.json"), 48000) as renderer:
    loss = ShortDrumLoss(reference, 48000)
    candidate, report = fit_parameters(
        renderer, reference,
        bounds={"model_level_db": (-60, 0)},
        objective=loss.residual, budget=60,
    )
```

This example exposes one coordinate only to demonstrate the API, not to suggest
that level-only fitting calibrates an instrument. Construct a reference-anchored
loss once and pass explicit coordinates/bounds. The driver searches normalized
coordinates with bounded Powell; it does not invent modes, fit per-mode T60,
enable EQ or expand the parameter set. The initial point remains a candidate.

The result is a new document with parent ID and a trace of every evaluation,
alongside reference PCM and native-library hashes. Nothing is auto-published or
written over a factory preset. Rendering preserves the saved gesture and seed;
separate multi-seed and multi-velocity audits are still needed before accepting
a real fit. A regression test first recovers a known native gain change and
checks that every other parameter is untouched.

This stage supplies the shared native fitting backend, not a mechanical port of
every old sample-server-dependent search/report script. Dataset selection and
experiment-specific optimizers can now be migrated against this backend without
bringing back Node. Plotly remains available for optional offline diagnostics.
