"""Audit the actual saved open-hat candidate; no automatic preset publication."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import numpy as np
from .artifacts import write_json
from .fit_open_hat import prepare
from .gong_onset import measure
from .hat_audition import export_state
from .layer_fit import LayerFit
from .plots import plots, spectrograms
from .saved import SavedFitRenderer
from .temporal_metal_loss import TemporalMetalLoss
from drumfoundry._native import library_path


def observation_level(saved, layers, parameters):
    """One visible output setting for the whole layer grid; preserve headroom."""
    audio = [saved.render(parameters, 3, 73519, layer["event"]) for layer in layers]
    target_energy = sum(np.sum(layer["reference"].window(3) ** 2) for layer in layers)
    actual_energy = sum(np.sum(x.astype(float) ** 2) for x in audio)
    desired = np.sqrt(target_energy / actual_energy)
    headroom = 0.8 / max(float(np.max(np.abs(x))) for x in audio)
    gain = min(desired, headroom)
    p = dict(parameters)
    p["model_level_db"] = min(0, parameters["model_level_db"] + 20 * np.log10(gain))
    if p["model_level_db"] < -60:
        raise ValueError("Output level cannot be represented")
    return p, dict(
        common_target_gain_db=float(20 * np.log10(desired)),
        chosen_model_level_db=p["model_level_db"],
        peak_target=0.8,
        per_layer_normalization=False,
    )


def restrikes(path):
    rows = []
    for rate in (44100, 48000):
        with SavedFitRenderer(path, rate) as voice:
            for strength in (0.25, 0.5, 0.76, 0.96):
                event = dict(voice.fit["controls"]["event"], strength=strength)
                for spacing in (0.125, 0.5):
                    a = voice.sequence(
                        voice.initial,
                        4,
                        [dict(event, time=i * spacing) for i in range(5)],
                    )
                    if not np.isfinite(a).all():
                        raise ValueError("Non-finite repeated hits")
                    rows.append(
                        dict(
                            rate=rate,
                            strength=strength,
                            spacing=spacing,
                            raw_peak=float(np.max(np.abs(a))),
                            peak_at_default_master=float(np.max(np.abs(a)))
                            * 10 ** (-12 / 20),
                        )
                    )
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for k in ("source", "root", "checkpoint", "output"):
        parser.add_argument("--" + k, type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    try:
        p = json.loads(args.checkpoint.read_text())["parameters"]
        trace_path = args.checkpoint.with_name(args.checkpoint.stem + "-trace.json")
        trace = json.loads(trace_path.read_text())
        seeds = sorted(
            {
                seed
                for row in trace
                for seed in row.get("layer_seeds", [row.get("seed", 1944)])
            }
        )
        p, gain = observation_level(saved, layers, p)
        for layer in layers:
            layer["loss"] = TemporalMetalLoss(layer["reference"].window(3), 44100)
        search = LayerFit(saved, layers, p)
        before = search.evaluate(saved.initial, "before", accept=False)
        search.evaluate(p, "candidate")
        write_json(args.output / "source.fit.json", saved.fit)
        export_state(
            search,
            args.output / "open",
            "open",
            dict(
                description="Structured series with limited low-core refinement; shared parameters across all velocities"
            ),
            training_seeds=seeds,
        )
        rows = []
        for seed in (73519, 47213, 91821):
            for layer in layers:
                row = dict(seed=seed, layer=layer["cell"]["velocity"])
                for name, parameters in (("before", saved.initial), ("candidate", p)):
                    audio = saved.render(parameters, 3, seed, layer["event"])
                    compared, _ = layer["loss"].comparison_audio(audio)
                    row[name] = dict(
                        layer["loss"].diagnostics(audio),
                        bands=measure(compared, 44100)[2],
                        raw_peak=float(np.max(np.abs(audio))),
                    )
                rows.append(row)
        layer = layers[2]
        reference = layer["reference"].window(3)
        candidate = saved.render(p, 3, 73519, layer["event"])
        old = saved.render(saved.initial, 3, 73519, layer["event"])
        plots(
            {"Reference": reference, "Before": old, "Candidate": candidate},
            44100,
            args.output,
            title="Open hat — actual saved output levels",
            seconds=3,
        )
        spectrograms(reference, candidate, 44100, args.output, seconds=3)
        import plotly.io as pio

        for file in args.output.glob("*.plotly.json"):
            pio.from_json(file.read_text()).write_image(str(file.with_suffix(".png")))
        checks = restrikes(args.output / "open/03-layer-096.fit.json")
        write_json(
            args.output / "audit.json",
            dict(
                parameters=p,
                output_level=gain,
                reference_bands=measure(reference, 44100)[2],
                independent_realizations=rows,
                repeated_hits=checks,
                source_sha256=sha256(args.source.read_bytes()).hexdigest(),
                native_sha256=sha256(library_path().read_bytes()).hexdigest(),
                objective=layers[0]["loss"].specification,
                acceptance="Candidate for audition, not certified perceptual equivalence",
            ),
        )
        print(
            json.dumps(
                dict(
                    before=before,
                    after=search.score,
                    model_level=p["model_level_db"],
                    max_monitor_peak=max(r["peak_at_default_master"] for r in checks),
                )
            ),
            flush=True,
        )
    finally:
        saved.close()


if __name__ == "__main__":
    main()
