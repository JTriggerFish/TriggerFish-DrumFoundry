"""Compare coherent-low / moving-high textures across velocities and two seeds."""

import argparse
import json
from pathlib import Path
from .fit_open_hat import prepare
from .layer_fit import LayerFit
from .temporal_metal_loss import TemporalMetalLoss
from .fit_temporal_hat import stage
from .artifacts import write_json
from .hat_audition import export_state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for k in ("source", "root", "checkpoint", "output"):
        parser.add_argument("--" + k, type=Path, required=True)
    parser.add_argument("--tail-only", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    for layer in layers:
        layer["loss"] = TemporalMetalLoss(layer["reference"].window(3), 44100)
    training = [
        dict(layer, training_seed=seed) for seed in (1944, 7823) for layer in layers
    ]
    p = json.loads(args.checkpoint.read_text())["parameters"]
    search = LayerFit(saved, training, p, workers=len(training))
    try:
        search.evaluate(p, "initial")
        if args.tail_only:
            stage(
                search,
                {
                    "body_decay_seconds_0": (0.2, 6, True),
                    "body_decay_seconds_7": (0.2, 6, True),
                    "body_brightness": (-24, 16, False),
                    "bloom_rate": (0.005, 16, True),
                    "bloom_energy_acceleration": (0, 1, False),
                    "low_prominence": (-3, 6, False),
                },
                "complete-decay",
                args.output,
                8,
            )
            search.layers = layers
            export_state(
                search,
                args.output / "open",
                "open",
                dict(description="Full 0-3 s temporal comparison; shared upper series"),
            )
            return
        for motion in (0.3, 0.7, 1.2):
            for blur in (0.001, 0.004, 0.012):
                for tilt in (0, 1):
                    search.evaluate(
                        p
                        | dict(
                            field_motion_depth=motion,
                            field_motion_rate=40,
                            field_phase_bandwidth=blur,
                            field_phase_tilt=tilt,
                        ),
                        "hybrid-texture",
                    )
        write_json(
            args.output / "grid.json", dict(parameters=search.best, score=search.score)
        )
        write_json(args.output / "grid-trace.json", search.rows)
        stage(
            search,
            {
                "low_prominence": (-4, 10, False),
                "field_phase_bandwidth": (0.0001, 0.03, True),
                "field_phase_tilt": (-1, 2, False),
                "field_motion_depth": (0, 2, False),
                "body_brightness": (-24, 16, False),
                "body_decay_seconds_0": (0.2, 6, True),
                "body_decay_seconds_7": (0.2, 6, True),
            },
            "definition-decay",
            args.output,
            8,
        )
        # Export the four actual reference layers, not duplicate training seeds.
        search.layers = layers
        export_state(
            search,
            args.output / "open",
            "open",
            dict(
                description="Retained upper series; low packet prominence/coherence and global texture"
            ),
        )
    finally:
        search.close()
        saved.close()


if __name__ == "__main__":
    main()
