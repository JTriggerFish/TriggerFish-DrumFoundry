"""Continue an inspected open-hat checkpoint, with independent native layer workers."""

import argparse
import json
import math
from .fit_open_hat import prepare, quiet_lower_series
from .parallel_layer_fit import ParallelLayerFit
from .artifacts import write_json
from .hat_audition import export_state
from pathlib import Path


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
    search = ParallelLayerFit(saved, layers, p)
    try:
        search.evaluate(saved.initial, "published", accept=False)
        search.evaluate(p, "checkpoint")
        for low in (-30, -24, -18, -12):
            search.evaluate(quiet_lower_series(p, 155, 3, low), "lower-harmonics")
        if args.focus == "body":
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
        write_json(
            args.output / "lower-harmonics.json",
            dict(score=search.score, parameters=search.best),
        )
        stages = [
            (
                "decay",
                {
                    "body_decay_seconds_0": (0.3, 7, True),
                    "body_decay_seconds_7": (0.3, 5, True),
                },
            ),
            (
                "texture",
                {
                    "field_turbulence": (0.3, 1.6, True),
                    "field_packet_spread": (0.3, 5, True),
                    "field_phase_bandwidth": (0.00002, 0.03, True),
                    "field_phase_tilt": (-1, 1, False),
                },
            ),
            (
                "balance",
                {
                    "body_brightness": (-8, 12, False),
                    "body_excitation_centre": (200, 7000, True),
                    "bloom_rate": (0, 6, False),
                },
            ),
            (
                "strike",
                {
                    "impact_tone_noise": (0.1, 1, False),
                    "impact_width": (0.25, 2, True),
                    "direct_gain": (0, 1, False),
                    "velocity_brightness": (0, 12, False),
                },
            ),
            (
                "decay-final",
                {
                    "body_decay_seconds_0": (0.3, 7, True),
                    "body_decay_seconds_7": (0.3, 5, True),
                },
            ),
        ]
        if args.focus == "body":
            stages = [
                (
                    "coherence",
                    {
                        "field_turbulence_slope": (-0.25, 1, False),
                        "field_phase_tilt": (-2, 1, False),
                        "field_phase_bandwidth": (0.00002, 0.03, True),
                    },
                ),
                (
                    "body",
                    {
                        "body_tune": (0.95, 1.06, False),
                        "body_brightness": (-8, 12, False),
                        "bloom_rate": (0, 3, False),
                    },
                ),
                (
                    "decay-final",
                    {
                        "body_decay_seconds_0": (0.3, 7, True),
                        "body_decay_seconds_7": (0.3, 5, True),
                    },
                ),
            ]
        if args.focus == "decay-curve":
            stages = [
                (
                    "decay-curve",
                    {
                        "body_decay_seconds_0": (0.5, 5, True),
                        "body_decay_seconds_3": (0.5, 5, True),
                        "body_decay_seconds_7": (0.3, 4, True),
                    },
                )
            ]
        for name, bounds in stages:
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
