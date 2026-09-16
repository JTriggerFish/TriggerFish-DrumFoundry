"""Fit three independent static hi-hat states; no runtime pedal changes.

Run with --source existing.fit.json --root reference-library --output fresh-dir.
Optional --publish creates a fresh user-preset subfolder, never replacing fits.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
from hashlib import sha256
import json
from pathlib import Path
from .saved import SavedFitRenderer
from .hat_layers import STATES, load_layers, starting_parameters
from .layer_fit import LayerFit
from .hat_audition import export_state
from .artifacts import write_json
from drumfoundry._native import library_path

STAGES = [
    (
        "decay-level",
        {
            "model_level_db": (-40, 0, False),
            "body_decay_seconds_0": (0.020001, 8, True),
            "body_decay_seconds_7": (0.020001, 8, True),
            "body_brightness": (-8, 20, False),
            "velocity_brightness": (0, 12, False),
        },
    ),
    (
        "attack-bloom",
        {
            "direct_gain": (0, 0.5, False),
            "impact_width": (0.25, 2, True),
            "impact_tone_noise": (0.5, 1, False),
            "body_excitation_centre": (800, 6500, True),
            "bloom_rate": (0, 5, False),
        },
    ),
    (
        "packet-texture",
        {
            "field_turbulence": (0.1, 16, True),
            "field_packet_spread": (0.3, 5, True),
            "field_phase_bandwidth": (0.001, 0.4, True),
            "field_turbulence_slope": (-0.3, 0.8, False),
        },
    ),
]


def run_state(args, state):
    """Independent native voice: states may fit in parallel, never share DSP state."""
    with SavedFitRenderer(args.source, 44100) as saved:
        layers = load_layers(saved.fit, args.catalog, args.root, state)
        search = LayerFit(saved, layers, starting_parameters(saved.initial, state))
        baseline = search.evaluate(saved.initial, "old-calibration", accept=False)
        print(f"{state}: old loss {baseline:.2f} dB", flush=True)
        for label, bounds in STAGES + [STAGES[0]]:
            score = search.stage(bounds, args.budget, label)
            print(
                f"{state}: {label} {score:.2f} dB ({len(search.rows)} trials)",
                flush=True,
            )
        export_state(search, args.output / state, state)
        return dict(state=state, before=baseline, after=search.score)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("source", "root", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    parser.add_argument("--publish", type=Path)
    parser.add_argument("--budget", type=int, default=100)
    args = parser.parse_args()
    if args.budget < 1:
        parser.error("Budget must be positive")
    args.catalog = json.loads((args.root / "catalog.json").read_text(encoding="utf8"))
    args.output.mkdir(parents=True, exist_ok=False)
    if args.publish is not None and args.publish.exists():
        parser.error("Publication requires a fresh preset folder")
    write_json(
        args.output / "source.fit.json",
        json.loads(args.source.read_text(encoding="utf8")),
    )
    write_json(
        args.output / "run.json",
        dict(
            source_sha256=sha256(args.source.read_bytes()).hexdigest(),
            native_sha256=sha256(library_path().read_bytes()).hexdigest(),
            stages=STAGES,
            budget_per_stage=args.budget,
            rate=44100,
            seconds=3,
            reference_gain_db=8,
            velocities="Fixed provisional catalog strengths; all four equally weighted",
        ),
    )
    with ThreadPoolExecutor(max_workers=3) as pool:
        results = list(pool.map(lambda state: run_state(args, state), STATES))
    write_json(args.output / "summary.json", results)
    if args.publish is not None:
        from .publish_hat_audition import publish

        publish(args.output, args.publish)
    print(results, flush=True)


if __name__ == "__main__":
    main()
