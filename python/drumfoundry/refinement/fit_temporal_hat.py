"""Gong-style open-hat refinement: shared series, causal phases, four velocities."""

import argparse
import json
from pathlib import Path
import numpy as np
from .artifacts import write_json
from .fit_open_hat import prepare
from .parallel_layer_fit import ParallelLayerFit
from .temporal_metal_loss import TemporalMetalLoss, residual_parts
from .fit_hat_body_ridge import prominence
from .least_squares import fit
from .hat_audition import export_state


def prominence_tilt(parameters, tilt):
    """One smooth editor-bar tilt; compensate common scaling in contact output."""
    p = parameters.copy()
    active = [i for i in range(32) if p[f"resolved_level_{i}"] > -72]
    levels = [
        p[f"resolved_level_{i}"] + tilt * np.log2(p[f"resolved_frequency_{i}"] / 1000)
        for i in active
    ]
    offset = max(0, max(levels) - 6)
    for i, level in zip(active, levels):
        p[f"resolved_level_{i}"] = max(-71, level - offset)
    p["direct_gain"] /= 10 ** (offset / 20)
    return p


def stage(search, bounds, label, output, iterations):
    """Finite differences expose which controls affect which temporal regions."""
    base = search.best.copy()
    names = list(bounds)
    limits = np.asarray([bounds[k][:2] for k in names], dtype=float)
    logs = np.asarray([bounds[k][2] for k in names], dtype=bool)
    limits[logs] = np.log(limits[logs])
    values = np.asarray([base.get(k, 0) for k in names])
    values[logs] = np.log(np.maximum(values[logs], 1e-20))
    start = np.clip((values - limits[:, 0]) / np.diff(limits, axis=1)[:, 0], 0, 1)

    def residual(x):
        values = limits[:, 0] + x * np.diff(limits, axis=1)[:, 0]
        values[logs] = np.exp(values[logs])
        changes = dict(zip(names, values))
        tilt = changes.pop("prominence_tilt", 0)
        boost = changes.pop("low_prominence", 0)
        parameters = prominence_tilt(base | changes, tilt)
        parameters = prominence(
            parameters,
            boost,
            parameters["resolved_frequency_0"],
            parameters["resolved_turbulence_0"],
        )

        def one(i):
            layer = search.layers[i]
            audio = search.voices[i].render(
                parameters,
                3,
                seed=layer.get("training_seed", 1944),
                event=layer["event"],
            )
            parts = layer["loss"].parts(audio)
            vector = residual_parts(parts)
            return vector, {k: float(np.sqrt(np.mean(a * a))) for k, a in parts.items()}

        results = list(search.pool.map(one, range(len(search.layers))))
        vector = np.concatenate([r[0] for r in results]) / np.sqrt(len(results))
        score = float(np.linalg.norm(vector))
        search.rows.append(
            dict(
                stage=label,
                score=score,
                parameters=parameters,
                components=[r[1] for r in results],
                layer_seeds=[
                    layer.get("training_seed", 1944) for layer in search.layers
                ],
            )
        )
        if score < search.score:
            search.score, search.best = score, parameters
        if len(search.rows) % 20 == 0:
            print(label, len(search.rows), "best", round(search.score, 4), flush=True)
        return vector

    report = fit(residual, start, iterations=iterations, step=0.01)
    write_json(
        output / (label + ".json"),
        dict(
            parameters=search.best,
            score=search.score,
            coordinates=names,
            bounds=bounds,
            solver=report,
        ),
    )
    write_json(output / (label + "-trace.json"), search.rows)
    print("FINISHED", label, search.score, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "root", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--checkpoint", type=Path)
    parser.add_argument("--family-trace", type=Path)
    parser.add_argument("--iterations", type=int, default=6)
    parser.add_argument("--skip-grid", action="store_true")
    parser.add_argument("--active-grid", action="store_true")
    parser.add_argument("--polish-body", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    for layer in layers:
        layer["loss"] = TemporalMetalLoss(layer["reference"].window(3), 44100)
    initial = (
        saved.initial
        if not args.checkpoint
        else json.loads(args.checkpoint.read_text())["parameters"]
    )
    if args.family_trace:
        rows = json.loads(args.family_trace.read_text())
        initial = min(
            (r for r in rows if r["stage"] == "gong-derived-family"),
            key=lambda r: r["score"],
        )["parameters"]
    search = ParallelLayerFit(saved, layers, initial)
    try:
        search.evaluate(initial, "initial")
        write_json(args.output / "source.fit.json", saved.fit)
        if args.polish_body:
            for boost in (0, 4, 8, 12):
                for coherence in (0, 0.3, 0.75):
                    p = prominence(
                        initial, boost, initial["resolved_frequency_0"], coherence
                    )
                    search.evaluate(p, "low-body-grid")
            stage(
                search,
                {
                    "low_prominence": (-4, 6, False),
                    "resolved_turbulence_0": (0, 1, False),
                    "field_motion_depth": (0.2, 2, False),
                    "field_motion_rate": (20, 200, True),
                    "body_decay_seconds_0": (0.2, 8, True),
                    "body_decay_seconds_7": (0.2, 6, True),
                    "body_brightness": (-24, 16, False),
                    "bloom_rate": (0.005, 16, True),
                },
                "tonal-body",
                args.output,
                args.iterations,
            )
            export_state(
                search,
                args.output / "open",
                "open",
                dict(
                    description="One dominant low packet; shared upper series retained"
                ),
            )
            return
        if args.active_grid:
            for tilt in (0, 3, 6):
                for excitation in (-12, -3, 4):
                    for concentration in (0, 0.3):
                        for energy in (0, 0.3):
                            candidate = prominence_tilt(initial, tilt) | dict(
                                body_brightness=excitation,
                                bloom_rate=8,
                                bloom_energy_acceleration=concentration,
                                bloom_energy_sensitivity=energy,
                            )
                            search.evaluate(candidate, "active-diffusion-grid")
        if not args.skip_grid:
            for tilt in (-12, 0, 4):
                for rate in (0.5, 3):
                    for concentration in (0.1, 0.7):
                        candidate = initial | dict(
                            body_brightness=tilt,
                            bloom_rate=rate,
                            bloom_energy_acceleration=concentration,
                        )
                        search.evaluate(candidate, "excitation-diffusion-grid")
        write_json(
            args.output / "grid.json", dict(parameters=search.best, score=search.score)
        )
        stage(
            search,
            {
                "prominence_tilt": (-6, 6, False),
                "body_brightness": (-24, 12, False),
                "body_excitation_centre": (300, 12000, True),
                "bloom_rate": (0.005, 16, True),
                "bloom_energy_acceleration": (0, 1, False),
                "bloom_energy_sensitivity": (0, 2, False),
                "body_decay_seconds_0": (0.2, 8, True),
                "body_decay_seconds_7": (0.2, 6, True),
            },
            "body-bloom-decay",
            args.output,
            args.iterations,
        )
        stage(
            search,
            {
                "impact_chirp_pitch": (0.25, 4, True),
                "impact_tone_noise": (0, 1, False),
                "impact_width": (0.25, 4, True),
                "impact_noise_tilt": (-24, 24, False),
                "direct_gain": (0, 2, False),
                "velocity_brightness": (0, 12, False),
            },
            "contact-velocity",
            args.output,
            args.iterations,
        )
        stage(
            search,
            {
                "field_turbulence": (0.15, 3, True),
                "field_turbulence_slope": (-0.5, 1, False),
                "field_packet_spread": (0.2, 5, True),
                "field_phase_bandwidth": (0.0001, 0.03, True),
                "field_phase_tilt": (-2, 2, False),
            },
            "texture",
            args.output,
            args.iterations,
        )
        stage(
            search,
            {
                "body_brightness": (-24, 12, False),
                "bloom_rate": (0.005, 16, True),
                "bloom_energy_acceleration": (0, 1, False),
                "body_decay_seconds_0": (0.2, 8, True),
                "body_decay_seconds_7": (0.2, 6, True),
            },
            "temporal-final",
            args.output,
            args.iterations,
        )
        export_state(
            search,
            args.output / "open",
            "open",
            dict(description="Retained stretched series; no ridge edits"),
        )
    finally:
        search.close()
        saved.close()


if __name__ == "__main__":
    main()
