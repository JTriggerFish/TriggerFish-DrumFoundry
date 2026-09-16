"""Gong-style open-hat refinement: shared series, causal phases, four velocities."""

import numpy as np
from .artifacts import write_json
from .temporal_metal_loss import residual_parts
from .fit_hat_body_ridge import prominence
from .least_squares import fit


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

        def one(loss, audio):
            parts = loss.parts(audio)
            vector = residual_parts(parts)
            return vector, {k: float(np.sqrt(np.mean(a * a))) for k, a in parts.items()}

        results = search.evaluation.measure(parameters, 1944, one)
        vector = np.concatenate([r[0] for r in results]) / np.sqrt(len(results))
        score = float(np.linalg.norm(vector))
        search.record(parameters, label, score, components=[r[1] for r in results])
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
