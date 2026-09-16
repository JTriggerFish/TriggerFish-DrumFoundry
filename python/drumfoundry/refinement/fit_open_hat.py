"""Restricted-series open-hat recalibration, all four layers at every evaluation."""

import argparse
from concurrent.futures import ThreadPoolExecutor
from hashlib import sha256
import json
from drumfoundry.editing import generate_series
from pathlib import Path
import numpy as np
from scipy.optimize import minimize
from .artifacts import write_json
from .hat_layers import load_layers
from .saved import SavedFitRenderer
from .tonal_layer_loss import TonalLayerLoss
from .hat_audition import export_state
from .layer_fit import LayerFit
from .open_hat_stages import INITIAL_STAGES
from drumfoundry._native import library_path


def series(parameters, base, stretch, first=1, tilt=-3):
    """Even stretched series and one smooth prominence tilt; no independent bars."""
    p = dict(parameters)
    frequencies = [
        m["frequency"]
        for m in generate_series(
            base, stretch, count=32, core=3, first=first, truncate=True
        )
    ]
    for i in range(32):
        f = frequencies[min(i, len(frequencies) - 1)]
        p[f"resolved_frequency_{i}"] = f
        p[f"resolved_level_{i}"] = (
            float(np.clip(tilt * np.log2(f / 1000), -36, 6))
            if i < len(frequencies)
            else -72
        )
        p[f"resolved_turbulence_{i}"] = 1
        p[f"resolved_allocation_{i}"] = 1
    return p


def quiet_lower_series(parameters, base, first, level=-18):
    """Retain quiet lower harmonics; do not erase measured low-frequency energy.

    All added low harmonics share one relative prominence, rather than giving
    the optimizer independent ridge frequencies or levels.
    """
    p = dict(parameters)
    free = [i for i in range(32) if p[f"resolved_level_{i}"] <= -72]
    anchor = p["resolved_level_0"]
    for i, harmonic in zip(free, range(1, first)):
        p[f"resolved_frequency_{i}"] = base * harmonic
        p[f"resolved_level_{i}"] = max(-72, anchor + level)
    return p


def prepare(source, root):
    """Each worker owns its native voice; references are fixed, gains analysis-only."""
    saved = SavedFitRenderer(source, 44100)
    catalog = json.loads((root / "catalog.json").read_text(encoding="utf8"))
    layers = load_layers(saved.fit, catalog, root, "open")
    for layer in layers:
        layer["loss"] = TonalLayerLoss(layer["reference"].window(3), 44100)
    return saved, layers


def evaluate_family(args, family):
    saved, layers = prepare(args.source, args.root)
    try:
        base, stretch, first, tilt, noise, blur = family
        p = series(saved.initial, base, stretch, first, tilt)
        p.update(
            field_turbulence=noise,
            field_phase_bandwidth=blur,
            field_phase_tilt=0,
            field_packet_spread=1.5,
            bloom_rate=0.3,
            body_brightness=2,
            body_decay_seconds_0=1.5,
            body_decay_seconds_7=1.8,
            direct_gain=0.2,
            velocity_brightness=4,
        )
        search = LayerFit(saved, layers, p)
        score = search.evaluate(p, "family")
        return dict(
            score=score,
            family=family,
            parameters=p,
            layers=search.rows[-1]["layer_errors_db"],
        )
    finally:
        saved.close()


def geometry_stage(search, geometry, budget):
    """Search only base, stretch and one whole-series tilt in normalized coordinates."""
    base, stretch, first, tilt, *_ = geometry
    initial = search.best.copy()
    limits = np.array(
        [[base * 0.88, base * 1.12], [max(0, stretch - 0.04), stretch + 0.04], [-8, 2]]
    )
    start = (np.array([base, stretch, tilt]) - limits[:, 0]) / np.diff(limits, axis=1)[
        :, 0
    ]
    best_geometry = [base, stretch, first, tilt]

    def objective(x):
        values = limits[:, 0] + x * np.diff(limits, axis=1)[:, 0]
        p = series(initial, values[0], values[1], first, values[2])
        old = search.score
        score = search.evaluate(p, "geometry")
        if score < old:
            best_geometry[:] = [
                float(values[0]),
                float(values[1]),
                first,
                float(values[2]),
            ]
        return score

    minimize(
        objective,
        start,
        method="Powell",
        bounds=[(0, 1)] * 3,
        options=dict(maxfev=budget, xtol=0.01, ftol=0.001),
    )
    return best_geometry


def choose_family(args):
    """Compare structured starting families, each with its own native voice."""
    families = [
        (base, stretch, first, tilt, noise, blur)
        for base, stretch, first in (
            (470, 0.1, 1),
            (235, 0.2, 2),
            (155, 0.28, 3),
            (520, 0.08, 1),
            (420, 0.12, 1),
        )
        for tilt in (-3, 0)
        for noise, blur in ((0.6, 0.01), (1, 0.003))
    ]
    with ThreadPoolExecutor(max_workers=4) as pool:
        results = []
        for result in pool.map(lambda f: evaluate_family(args, f), families):
            results.append(result)
            print(
                "family",
                result["family"],
                "score",
                round(result["score"], 3),
                flush=True,
            )
    write_json(args.output / "families.json", results)
    return min(results, key=lambda r: r["score"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "root", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--budget", type=int, default=140)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    write_json(
        args.output / "run.json",
        dict(
            native_sha256=sha256(library_path().read_bytes()).hexdigest(),
            source_sha256=sha256(args.source.read_bytes()).hexdigest(),
            budget=args.budget,
        ),
    )
    winner = choose_family(args)
    saved, layers = prepare(args.source, args.root)
    try:
        search = LayerFit(saved, layers, winner["parameters"])
        before = search.evaluate(saved.initial, "published", accept=False)
        search.evaluate(winner["parameters"], "best-family")
        geometry = winner["family"]
        for label, bounds in INITIAL_STAGES:
            search.stage(bounds, args.budget, label)
            print(label, search.score, flush=True)
            write_json(
                args.output / (label + ".json"),
                dict(score=search.score, parameters=search.best, bounds=bounds),
            )
        geometry = geometry_stage(search, geometry, args.budget)
        print("geometry", geometry, search.score, flush=True)
        search.stage(INITIAL_STAGES[1][1], args.budget, "decay-revisit")
        write_json(args.output / "source.fit.json", saved.fit)
        write_json(
            args.output / "summary.json",
            dict(before=before, after=search.score, geometry=geometry),
        )
        export_state(
            search,
            args.output / "open",
            "open",
            dict(
                base=geometry[0],
                stretch=geometry[1],
                first=geometry[2],
                tilt=geometry[3],
            ),
        )
        print("DONE", before, search.score, flush=True)
    finally:
        saved.close()


if __name__ == "__main__":
    main()
