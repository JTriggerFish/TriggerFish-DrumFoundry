"""Inspect a calibration checkpoint against all layers, without publishing it."""

import argparse
import json
from pathlib import Path
import numpy as np
from .fit_open_hat import prepare
from .artifacts import write_json
from .plots import plots, spectrograms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("source", "root", "checkpoint", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    try:
        parameters = json.loads(args.checkpoint.read_text(encoding="utf8"))[
            "parameters"
        ]
        rows, renders = [], []
        for layer in layers:
            audio = saved.render(parameters, 3, seed=73519, event=layer["event"])
            renders.append(audio)
            rows.append(layer["loss"].diagnostics(audio))
        # Proposed *single* visible model-level adjustment, not individual gains.
        target_energy = sum(
            float(np.sum(l["reference"].window(3) ** 2)) for l in layers
        )
        model_energy = sum(float(np.sum(a.astype(float) ** 2)) for a in renders)
        gain = float(np.sqrt(target_energy / model_energy))
        # Equivalent, visible observation restaging: no extra body excitation.
        headroom = min(
            4 / parameters["field_gain"], 2 / max(parameters["direct_gain"], 1e-10)
        )
        proposed = dict(
            parameters,
            field_gain=parameters["field_gain"] * headroom,
            direct_gain=parameters["direct_gain"] * headroom,
            model_level_db=parameters["model_level_db"]
            + 20 * np.log10(gain / headroom),
        )
        if not -60 <= proposed["model_level_db"] <= 0:
            raise ValueError(
                "Common output gain cannot be represented within native limits"
            )
        level = float(proposed["model_level_db"])
        reference = layers[2]["reference"].window(3)
        before = saved.render(saved.initial, 3, seed=73519, event=layers[2]["event"])
        plots(
            {"Reference": reference, "Before": before, "Candidate": renders[2] * gain},
            44100,
            args.output,
            title="Open hat; candidate uses proposed saved model level",
            seconds=3,
        )
        spectrograms(reference, renders[2] * gain, 44100, args.output, seconds=3)
        write_json(
            args.output / "audit.json",
            dict(
                layers=rows,
                proposed_parameters=proposed,
                proposed_model_level_db=level,
                common_gain_db=20 * np.log10(gain),
                proposed_raw_peaks=[float(np.max(abs(a))) * gain for a in renders],
            ),
        )
        import plotly.io as pio

        for file in args.output.glob("*.plotly.json"):
            pio.from_json(file.read_text(encoding="utf8")).write_image(
                str(file.with_suffix(".png"))
            )
        print(json.dumps(dict(layers=rows, proposed_model_level_db=level)), flush=True)
    finally:
        saved.close()


if __name__ == "__main__":
    main()
