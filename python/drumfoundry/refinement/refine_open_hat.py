"""Continue an inspected open-hat checkpoint, with independent native layer workers."""

import argparse
import json
import math
from .fit_open_hat import prepare, quiet_lower_series
from .layer_fit import LayerFit
from .artifacts import write_json
from .hat_audition import export_state
from pathlib import Path
from .open_hat_stages import stages_for


def seed_decay_curve(p):
    """Insert one interior knot without changing the starting decay curve."""
    erb = lambda f: 21.4 * math.log10(1 + 0.00437 * f)
    upper = p.get("body_decay_frequency_7", 15000)
    amount = (erb(4000) - erb(40)) / (erb(upper) - erb(40))
    seconds = math.exp(
        (1 - amount) * math.log(p["body_decay_seconds_0"])
        + amount * math.log(p["body_decay_seconds_7"])
    )
    p.update(
        body_decay_active_3=1,
        body_decay_frequency_3=4000,
        body_decay_seconds_3=seconds,
    )


def explore_body(search, p):
    """Compare smooth coherence profiles, not independent upper ridges."""
    for scale in (0.25, 0.5, 0.75):
        candidate = p.copy()
        for i in range(32):
            if p[f"resolved_frequency_{i}"] < 800:
                candidate[f"resolved_turbulence_{i}"] = scale
        search.evaluate(candidate, "low-packet-coherence")
    for slope in (0, 0.25, 0.5, 0.75, 1):
        # Keep phase-bandwidth tilt approximately unchanged while
        # testing a more coherent low packet / broader upper packets.
        tilt = p["field_phase_tilt"] - 2 * (slope - p["field_turbulence_slope"])
        search.evaluate(
            p
            | {
                "field_turbulence_slope": slope,
                "field_phase_tilt": max(-2, tilt),
            },
            "coherence-profile",
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("source", "root", "checkpoint", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    parser.add_argument("--budget", type=int, default=100)
    parser.add_argument(
        "--focus", choices=("full", "body", "decay-curve"), default="full"
    )
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    p = json.loads(args.checkpoint.read_text(encoding="utf8"))["parameters"]
    if args.focus == "decay-curve":
        seed_decay_curve(p)
    search = LayerFit(saved, layers, p, workers=len(layers))
    try:
        search.evaluate(saved.initial, "published", accept=False)
        search.evaluate(p, "checkpoint")
        for low in (-30, -24, -18, -12):
            search.evaluate(quiet_lower_series(p, 155, 3, low), "lower-harmonics")
        if args.focus == "body":
            explore_body(search, p)
        write_json(
            args.output / "lower-harmonics.json",
            dict(score=search.score, parameters=search.best),
        )
        for name, bounds in stages_for(args.focus):
            search.stage(bounds, args.budget, name)
            write_json(
                args.output / (name + ".json"),
                dict(score=search.score, parameters=search.best, bounds=bounds),
            )
            write_json(args.output / (name + "-trials.json"), search.rows)
            print("FINISHED", name, search.score, flush=True)
        write_json(args.output / "source.fit.json", saved.fit)
        export_state(
            search, args.output / "open", "open", dict(base=155, stretch=0.28, first=3)
        )
    finally:
        search.close()
        saved.close()


if __name__ == "__main__":
    main()
