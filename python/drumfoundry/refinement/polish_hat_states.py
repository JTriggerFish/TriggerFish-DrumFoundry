"""Refine static states: absolute level/decay, or explicitly level-invariant timbre."""

import argparse
from concurrent.futures import ThreadPoolExecutor
from hashlib import sha256
import json
from pathlib import Path
from drumfoundry._native import library_path
from .artifacts import write_json
from .saved import SavedFitRenderer
from .hat_layers import STATES, load_layers
from .layer_fit import LayerFit
from .hat_audition import export_state

BOUNDS = {
    "model_level_db": (-40, 0, False),
    "body_decay_seconds_0": (0.020001, 8, True),
    "body_decay_seconds_7": (0.020001, 8, True),
}
TIMBRE_BOUNDS = {
    "body_decay_seconds_0": (0.020001, 8, True),
    "body_decay_seconds_7": (0.020001, 8, True),
    "body_brightness": (-8, 20, False),
    "field_turbulence_slope": (-0.3, 0.8, False),
    "field_phase_bandwidth": (0.001, 0.4, True),
    "velocity_brightness": (0, 12, False),
}


def polish(args, state):
    """Keep geometry frozen; choose explicitly between level and timbre fitting."""
    path = sorted((args.run / state).glob("*.fit.json"))[2]
    previous = json.loads((args.run / state / "report.json").read_text(encoding="utf8"))
    with SavedFitRenderer(path, 44100) as saved:
        layers = load_layers(
            saved.fit,
            args.catalog,
            args.root,
            state,
            level_policy="timbre" if args.timbre else "absolute",
        )
        search = LayerFit(saved, layers, saved.initial)
        before = search.evaluate(saved.initial, "before")
        bounds = TIMBRE_BOUNDS if args.timbre else BOUNDS
        search.stage(
            bounds, args.budget, "timbre-only" if args.timbre else "joint-level-decay"
        )
        export_state(search, args.output / state, state, previous["geometry"])
        result = dict(
            state=state,
            before=before,
            after=search.score,
            source_fit_sha256=sha256(path.read_bytes()).hexdigest(),
        )
        print(result, flush=True)
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("run", "root", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    parser.add_argument("--budget", type=int, default=100)
    parser.add_argument(
        "--timbre",
        action="store_true",
        help="Ignore one overall level offset per layer in analysis only; freeze playback gain",
    )
    args = parser.parse_args()
    if args.budget < 1:
        parser.error("Budget must be positive")
    args.output.mkdir(parents=True, exist_ok=False)
    args.catalog = json.loads((args.root / "catalog.json").read_text(encoding="utf8"))
    write_json(
        args.output / "source.fit.json",
        json.loads((args.run / "source.fit.json").read_text(encoding="utf8")),
    )
    with ThreadPoolExecutor(max_workers=3) as pool:
        results = list(pool.map(lambda state: polish(args, state), STATES))
    write_json(
        args.output / "polish.json",
        dict(
            results=results,
            bounds=TIMBRE_BOUNDS if args.timbre else BOUNDS,
            level_policy="timbre" if args.timbre else "absolute",
            budget=args.budget,
            parent=str(args.run.resolve()),
            native_sha256=sha256(library_path().read_bytes()).hexdigest(),
        ),
    )


if __name__ == "__main__":
    main()
