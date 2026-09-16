"""Test shared pitch/stretch against all three states and all four velocities."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
from hashlib import sha256
import numpy as np
from drumfoundry._native import library_path
from .saved import SavedFitRenderer
from .hat_layers import STATES, load_layers
from .layer_series import stretched_series, observation_headroom
from .layer_fit import LayerFit
from .hat_audition import export_state
from .artifacts import write_json


def run(args):
    """Keep geometry common; reject a tradeoff that worsens any state over .25 dB."""
    args.output.mkdir(parents=True, exist_ok=False)
    catalog = json.loads((args.root / "catalog.json").read_text(encoding="utf8"))
    searches = []
    input_hashes = {}
    try:
        for state in STATES:
            path = sorted((args.run / state).glob("*.fit.json"))[2]
            input_hashes[state] = sha256(path.read_bytes()).hexdigest()
            saved = SavedFitRenderer(path, 44100)
            layers = load_layers(saved.fit, catalog, args.root, state)
            searches.append(
                LayerFit(saved, layers, observation_headroom(saved.initial))
            )
        rows = []
        with ThreadPoolExecutor(max_workers=3) as pool:
            list(
                pool.map(
                    lambda s: s.stage(
                        {"model_level_db": (-40, 0, False)},
                        30,
                        "observation-headroom-level",
                    ),
                    searches,
                )
            )
            bases = [dict(s.best) for s in searches]
            for fundamental, stretch in [(260, 0.22)] + [
                (f, s) for f in (220, 240, 260, 280, 300) for s in (0.14, 0.22, 0.26)
            ]:
                values = [
                    stretched_series(base, fundamental, stretch, level=0)
                    for base in bases
                ]
                errors = list(
                    pool.map(
                        lambda pair: pair[0].evaluate(
                            pair[1],
                            f"shared-series/{fundamental}/{stretch}",
                            accept=False,
                        ),
                        zip(searches, values),
                    )
                )
                row = dict(
                    fundamental=fundamental,
                    stretch=stretch,
                    errors=errors,
                    score=float(np.sqrt(np.mean(np.square(errors)))),
                )
                rows.append(row)
                print(row, flush=True)
        baseline = np.array(rows[0]["errors"])
        best = min(
            (r for r in rows if np.all(np.array(r["errors"]) <= baseline + 0.25)),
            key=lambda r: r["score"],
        )
        geometry = dict(
            fundamental=best["fundamental"],
            stretch=best["stretch"],
            count=24,
            harmonic_core=3,
            modal_level_db=0,
        )
        for state, search, base in zip(STATES, searches, bases):
            values = stretched_series(
                base, best["fundamental"], best["stretch"], level=0
            )
            search.best = values
            search.score = search.evaluate(
                values, "selected-shared-series", accept=False
            )
            search.stage(
                {"model_level_db": (-40, 0, False)}, 30, "final-common-series-level"
            )
            export_state(search, args.output / state, state, geometry)
        write_json(
            args.output / "source.fit.json",
            json.loads((args.run / "source.fit.json").read_text(encoding="utf8")),
        )
        write_json(
            args.output / "series-search.json",
            dict(
                candidates=rows,
                selected=best,
                state_regression_limit_db=0.25,
                parent=str(args.run.resolve()),
                input_fits_sha256=input_hashes,
                native_sha256=sha256(library_path().read_bytes()).hexdigest(),
                observation_headroom_db=12,
                level_bounds_db=[-40, 0],
            ),
        )
    finally:
        for search in searches:
            search.saved.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("run", "root", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
