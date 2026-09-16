"""Gong-style open-hat refinement using shared native layer evaluation."""

import argparse
import json
from pathlib import Path
from .artifacts import write_json
from .fit_open_hat import prepare
from .layer_fit import LayerFit
from .temporal_metal_loss import TemporalMetalLoss
from .fit_hat_body_ridge import prominence
from .hat_audition import export_state
from .temporal_hat_stage import stage, prominence_tilt
from .temporal_hat_stages import STAGES


def arguments():
    """Parse one explicit experiment configuration."""
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "root", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--checkpoint", type=Path)
    parser.add_argument("--family-trace", type=Path)
    parser.add_argument("--iterations", type=int, default=6)
    parser.add_argument("--skip-grid", action="store_true")
    parser.add_argument("--active-grid", action="store_true")
    parser.add_argument("--polish-body", action="store_true")
    return parser.parse_args()


def starting_parameters(args, saved):
    """Warm start from a checkpoint, family winner or the saved input."""
    initial = (
        saved.initial
        if not args.checkpoint
        else json.loads(args.checkpoint.read_text())["parameters"]
    )
    if args.family_trace:
        rows = json.loads(args.family_trace.read_text())
        initial = min(
            (r for r in rows if r["stage"] == "gong-derived-family"),
            key=lambda r: r["score"],
        )["parameters"]
    return initial


def explore_grid(search, initial, args):
    """Compare shared excitation and transport regimes before local fitting."""
    if args.active_grid:
        for tilt in (0, 3, 6):
            for excitation in (-12, -3, 4):
                for concentration in (0, 0.3):
                    for energy in (0, 0.3):
                        candidate = prominence_tilt(initial, tilt) | dict(
                            body_brightness=excitation,
                            bloom_rate=8,
                            bloom_energy_acceleration=concentration,
                            bloom_energy_sensitivity=energy,
                        )
                        search.evaluate(candidate, "active-diffusion-grid")
    if not args.skip_grid:
        for tilt in (-12, 0, 4):
            for rate in (0.5, 3):
                for concentration in (0.1, 0.7):
                    candidate = initial | dict(
                        body_brightness=tilt,
                        bloom_rate=rate,
                        bloom_energy_acceleration=concentration,
                    )
                    search.evaluate(candidate, "excitation-diffusion-grid")
    write_json(
        args.output / "grid.json", dict(parameters=search.best, score=search.score)
    )


def run_stages(search, initial, args):
    """Keep body-polish and the ordinary causal sequence distinct."""
    if args.polish_body:
        for boost in (0, 4, 8, 12):
            for coherence in (0, 0.3, 0.75):
                p = prominence(
                    initial, boost, initial["resolved_frequency_0"], coherence
                )
                search.evaluate(p, "low-body-grid")
        names = ("tonal-body",)
    else:
        explore_grid(search, initial, args)
        names = ("body-bloom-decay", "contact-velocity", "texture", "temporal-final")
    for name in names:
        stage(search, STAGES[name], name, args.output, args.iterations)


def main():
    args = arguments()
    args.output.mkdir(parents=True, exist_ok=False)
    saved, layers = prepare(args.source, args.root)
    try:
        for layer in layers:
            layer["loss"] = TemporalMetalLoss(layer["reference"].window(3), 44100)
        initial = starting_parameters(args, saved)
        with LayerFit(saved, layers, initial, workers=len(layers)) as search:
            search.evaluate(initial, "initial")
            write_json(args.output / "source.fit.json", saved.fit)
            run_stages(search, initial, args)
            description = (
                "One dominant low packet; shared upper series retained"
                if args.polish_body
                else "Retained stretched series; no ridge edits"
            )
            export_state(
                search, args.output / "open", "open", dict(description=description)
            )
    finally:
        saved.close()


if __name__ == "__main__":
    main()
