"""Test gong-derived excitation/transfer families without borrowing its tuning."""

import argparse
import json
from pathlib import Path
from .fit_open_hat import prepare
from .fit_temporal_hat import prominence_tilt
from .layer_fit import LayerFit
from .temporal_metal_loss import TemporalMetalLoss
from .artifacts import write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("source", "root", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    for layer in layers:
        layer["loss"] = TemporalMetalLoss(layer["reference"].window(3), 44100)
    search = LayerFit(saved, layers, saved.initial, workers=len(layers))
    try:
        search.evaluate(saved.initial, "published")
        for brightness in (-28, -12, 0):
            for centre in (500, 1500):
                for prominence in (0, 3, 6):
                    p = prominence_tilt(saved.initial, prominence) | dict(
                        body_brightness=brightness,
                        body_excitation_centre=centre,
                        body_excitation=3,
                        bloom_rate=8,
                        bloom_energy_acceleration=0.05,
                        bloom_energy_sensitivity=0.3,
                        field_distribution=3,
                        field_turbulence=0.70625,
                        field_turbulence_slope=0.4,
                        field_packet_spread=1.8,
                        field_phase_bandwidth=0,
                        field_motion_depth=1.5,
                        field_motion_rate=200,
                        field_motion_sharing=0.15,
                        impact_chirp_pitch=3.09,
                        impact_tone_noise=0.116,
                        impact_noise_tilt=-8.56,
                        direct_gain=0,
                    )
                    search.evaluate(p, "gong-derived-family")
        write_json(
            args.output / "best.json", dict(parameters=search.best, score=search.score)
        )
        write_json(args.output / "trace.json", search.rows)
        print("DONE", search.score, flush=True)
    finally:
        search.close()
        saved.close()


if __name__ == "__main__":
    main()
