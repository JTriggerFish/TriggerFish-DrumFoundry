"""One dominant low packet may refine a series; upper handles remain coupled."""

import argparse
import json
from pathlib import Path
import numpy as np
from scipy.optimize import minimize
from .fit_open_hat import prepare
from .layer_fit import LayerFit
from .artifacts import write_json
from .hat_audition import export_state


def prominence(parameters, boost, frequency, coherence):
    """Relative dominant-packet prominence without exceeding native bar limits."""
    p = dict(parameters)
    p["resolved_frequency_0"] = frequency
    p["resolved_turbulence_0"] = coherence
    # Scale contact with the other observation paths. Otherwise a relative
    # low-packet boost also raises contact/body ratio, contaminating the attack.
    p["direct_gain"] /= 10 ** (boost / 20)
    for i in range(1, 32):
        if p[f"resolved_level_{i}"] > -72:
            p[f"resolved_level_{i}"] = max(-71, p[f"resolved_level_{i}"] - boost)
    return p


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("source", "root", "checkpoint", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    base = json.loads(args.checkpoint.read_text(encoding="utf8"))["parameters"]
    search = LayerFit(saved, layers, base, workers=len(layers))
    try:
        search.evaluate(base, "before")
        bounds = np.array([[0, 18], [430, 510], [0.15, 1.0]])
        start = (
            np.array([0, base["resolved_frequency_0"], base["resolved_turbulence_0"]])
            - bounds[:, 0]
        ) / np.diff(bounds, axis=1)[:, 0]

        def objective(x):
            v = bounds[:, 0] + x * np.diff(bounds, axis=1)[:, 0]
            return search.evaluate(prominence(base, *v), "dominant-packet")

        minimize(
            objective,
            start,
            method="Powell",
            bounds=[(0, 1)] * 3,
            options=dict(maxfev=110, xtol=0.008, ftol=0.001),
        )
        write_json(
            args.output / "prominence.json",
            dict(score=search.score, parameters=search.best),
        )
        search.stage(
            {
                "body_decay_seconds_0": (0.5, 7, True),
                "body_decay_seconds_7": (0.5, 5, True),
            },
            90,
            "decay",
        )
        write_json(
            args.output / "decay.json", dict(score=search.score, parameters=search.best)
        )
        write_json(args.output / "source.fit.json", saved.fit)
        export_state(
            search,
            args.output / "open",
            "open",
            dict(
                base=155,
                stretch=0.28,
                refinement="One dominant low packet frequency/coherence; common prominence relative to remaining series",
            ),
        )
    finally:
        search.close()
        saved.close()


if __name__ == "__main__":
    main()
