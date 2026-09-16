"""Independent diagnostics for static hi-hat candidates; no automatic acceptance."""

import argparse
import json
from pathlib import Path
import numpy as np
from .artifacts import write_json
from .hat_layers import STATES
from .saved import SavedFitRenderer
from .reference import load_reference
from .plots import plots, spectrograms
from .ridge_contrast import ridge_contrast
from .layer_loss import LayerLoss
from triggerfish_percussion.modal_texture_loss import ModalTextureLoss


def inspect_state(run, root, output, state, png=False, perceptual=False):
    """Reference, baseline and candidate retain the same gain and strike strength."""
    directory = output / state
    directory.mkdir(parents=True, exist_ok=False)
    fit_path = sorted((run / state).glob("*.fit.json"))[2]
    with SavedFitRenderer(fit_path, 44100) as voice:
        fit = voice.fit
        ref = load_reference(fit, root / fit["reference"]["libraryPath"], 44100)
        reference = ref.window(3)
        event = fit["controls"]["event"]
        audio = voice.render(voice.initial, 3)
        with SavedFitRenderer(run / "source.fit.json", 44100) as baseline:
            before = baseline.render(baseline.initial, 3, event=event)
        plots(
            {"Reference": reference, "Before": before, "Candidate": audio},
            44100,
            directory,
            title=f"Hi-hat {state}",
            seconds=3,
        )
        spectrograms(reference, audio, 44100, directory, seconds=3)
        rows = []
        for spacing in (0.125, 0.5):
            hits = [dict(event, time=i * spacing) for i in range(5)]
            sequence = voice.sequence(voice.initial, 5, hits)
            if not np.isfinite(sequence).all():
                raise ValueError("Non-finite repeated-hit render")
            rows.append(dict(spacing=spacing, raw_peak=float(np.max(np.abs(sequence)))))
        texture = ModalTextureLoss(reference, 44100)
        results = dict(
            reference=ref.provenance,
            repeated_hits=rows,
            texture=dict(
                before=texture.score(before),
                candidate=texture.score(audio),
                specification=texture.specification,
            ),
            ridge_contrast={
                name: ridge_contrast(signal, 44100)
                for name, signal in (
                    ("reference", reference),
                    ("before", before),
                    ("candidate", audio),
                )
            },
        )
        if perceptual:
            import torch
            from triggerfish_percussion.reference_floor_mel import ReferenceFloorMel

            torch.set_num_threads(1)
            loss = ReferenceFloorMel(reference, 44100)
            timbre = LayerLoss(reference, 44100, "timbre")
            shape_before, offset_before = timbre.comparison_audio(before)
            shape_candidate, offset_candidate = timbre.comparison_audio(audio)
            results["heldout_mel"] = dict(
                before=loss.score(before),
                candidate=loss.score(audio),
                timbre_before=loss.score(shape_before),
                timbre_candidate=loss.score(shape_candidate),
                analysis_level_offsets_db=[offset_before, offset_candidate],
                specification=loss.specification,
            )
        write_json(directory / "audit.json", results)
    if png:
        import plotly.io as pio

        for path in directory.glob("*.plotly.json"):
            fig = pio.from_json(path.read_text(encoding="utf8"))
            fig.write_image(str(path.with_suffix(".png")))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("run", "root", "output"):
        parser.add_argument("--" + key, type=Path, required=True)
    parser.add_argument("--png", action="store_true", help="Requires optional Kaleido")
    parser.add_argument(
        "--perceptual", action="store_true", help="Requires perceptual-fit dependencies"
    )
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    for state in STATES:
        inspect_state(
            args.run, args.root, args.output, state, args.png, args.perceptual
        )


if __name__ == "__main__":
    main()
