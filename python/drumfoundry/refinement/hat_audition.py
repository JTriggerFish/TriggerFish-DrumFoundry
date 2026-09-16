"""Export versioned local audition presets and raw, unnormalized comparison WAVs."""

from copy import deepcopy
from hashlib import sha256
import numpy as np
from .artifacts import write_json
from .saved import SavedFitRenderer
from triggerfish_percussion.audio_io import AudioBuffer, write_wav


def export_state(search, output, state, geometry=None, training_seeds=None):
    """Freeze and verify velocity audition views before any local publication."""
    output.mkdir(parents=True, exist_ok=False)
    reference_audio, model_audio = [], []
    rows = []
    for index, layer in enumerate(search.layers):
        fit = search.saved.snapshot(search.best, f"Hi-hat — {state} — layer {index+1}")
        fit["reference"] = deepcopy(layer["attachment"])
        fit["controls"]["event"].update(layer["event"], seed=73519)
        fit["reviewStatus"] = "multi-layer-candidate-audition-pending"
        fit["reviewNote"] = (
            "Shared stretched series; provisional catalog strengths, not verified recorded MIDI velocities. No per-layer playback matching or output EQ."
        )
        fit["reviewNote"] += (
            " Analysis level policy: " + layer["loss"].level_policy + "."
        )
        name = f"{index+1:02d}-layer-{layer['cell']['velocity']:03d}.fit.json"
        write_json(output / name, fit)
        audio = search.saved.render(search.best, 3, seed=73519, event=layer["event"])
        target = layer["reference"].window(3)
        with SavedFitRenderer(output / name, search.saved.sample_rate) as reload:
            repeated = reload.render(reload.initial, 3)
        if not np.array_equal(audio, repeated):
            raise ValueError("Saved candidate does not reproduce its audition render")
        reference_audio.append(target)
        model_audio.append(audio)
        diagnostics = layer["loss"].diagnostics(audio)
        rows.append(
            dict(
                layer=index + 1,
                reference=layer["reference"].provenance,
                event=layer["event"],
                heldout_error_db=diagnostics["error_db"],
                analysis_level_offset_db=diagnostics.get("comparison_gain_db", 0),
                raw_peak=float(np.max(np.abs(audio))),
                reference_peak=float(np.max(np.abs(target))),
                energy_error_db=float(
                    10 * np.log10(np.sum(audio**2) / np.sum(target**2))
                ),
                fit_sha256=sha256((output / name).read_bytes()).hexdigest(),
            )
        )
    # Same parameter vector, velocity-specific reference/strike defaults only.
    for name, signals in (("reference", reference_audio), ("candidate", model_audio)):
        write_wav(
            output / f"{name}-velocities.wav",
            AudioBuffer(np.concatenate(signals), search.saved.sample_rate),
        )
    write_json(
        output / "report.json",
        dict(
            state=state,
            training_score_db=search.score,
            objective=search.layers[0]["loss"].specification,
            layers=rows,
            training_seed=1944,
            training_seeds=training_seeds
            or sorted(
                {
                    seed
                    for row in search.rows
                    for seed in row.get("layer_seeds", [row.get("seed", 1944)])
                }
            ),
            heldout_seed=73519,
            geometry=geometry
            or dict(fundamental=260, stretch=0.22, count=24, harmonic_core=3),
            parameters=search.best,
            exact_reload=True,
            acceptance="Requires user audition; lower numerical loss is not sound approval",
        ),
    )
    write_json(output / "trace.json", search.rows)
