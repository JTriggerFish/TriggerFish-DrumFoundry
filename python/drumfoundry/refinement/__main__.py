"""Native fitting entry point: python -m drumfoundry.refinement --help."""

import argparse
from pathlib import Path
from .run import run


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--fit", type=Path, required=True)
    p.add_argument("--reference", type=Path, required=True)
    p.add_argument(
        "--library-root", type=Path, help="Reference library root for a new attachment"
    )
    p.add_argument("--output", type=Path, required=True)
    p.add_argument(
        "--profile",
        choices=("metal", "kick", "short-drum", "gong-onset"),
        default="metal",
    )
    p.add_argument(
        "--stage",
        choices=(
            "audit-only",
            "locked-texture",
            "shared-series",
            "sparse-decay",
            "fine-decay",
            "upper-balance",
            "low-decay",
            "coordinates",
            "gong-grid",
        ),
        default="locked-texture",
    )
    p.add_argument("--rate", type=int, default=48000)
    p.add_argument("--seconds", type=float)
    p.add_argument("--budget", type=int, default=180)
    p.add_argument(
        "--solver",
        choices=("powell", "least-squares"),
        default="powell",
        help="Least squares is for explicit coordinates and vector losses",
    )
    p.add_argument(
        "--difference-step",
        type=float,
        default=0.005,
        help="Absolute finite-difference step as a fraction of each bound range",
    )
    p.add_argument(
        "--bounds",
        type=Path,
        help="Explicit coordinate bounds; no automatic mode edits",
    )
    p.add_argument("--name", default="Refinement candidate")
    p.add_argument("--gain", type=float, help="Override saved reference gain (dB)")
    p.add_argument(
        "--onset", type=float, help="Override saved reference onset (seconds)"
    )
    p.add_argument(
        "--channel", type=int, choices=(0, 1, 2), help="0 mean, 1 left, 2 right"
    )
    p.add_argument(
        "--plots",
        action="store_true",
        help="Write optional fixed-scale Plotly diagnostics",
    )
    return p


if __name__ == "__main__":
    p = parser()
    args = p.parse_args()
    try:
        run(args)
    except ValueError as error:
        p.error(str(error))
