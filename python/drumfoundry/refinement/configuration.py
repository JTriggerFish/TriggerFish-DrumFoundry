"""Validate a complete experiment before rendering or creating output files."""

from copy import copy
import json


def duration(profile):
    """Analysis regions are fixed for each declared objective."""
    return {"metal": 6.0, "gong-onset": 3.0, "kick": 1.2, "short-drum": 1.2}[profile]


def validated(args):
    """Freeze bounds once and reject incompatible or silently ignored options."""
    args = copy(args)
    expected = duration(args.profile)
    if args.seconds is None:
        args.seconds = expected
    if args.seconds != expected:
        raise ValueError(f"{args.profile} requires {expected} seconds")
    if type(args.budget) is not int or args.budget < 0:
        raise ValueError("Budget must be a nonnegative integer")
    args.solver = getattr(args, "solver", "powell")
    if args.solver not in ("powell", "least-squares"):
        raise ValueError("Unknown solver")
    allowed = {"audit-only", "coordinates"}
    if args.profile == "metal":
        allowed.update(
            (
                "locked-texture",
                "shared-series",
                "sparse-decay",
                "fine-decay",
                "upper-balance",
                "low-decay",
            )
        )
    elif args.profile == "gong-onset":
        allowed.add("gong-grid")
    if args.stage not in allowed:
        raise ValueError("Incompatible profile and stage")
    if args.solver == "least-squares":
        if args.stage != "coordinates" or args.profile == "metal":
            raise ValueError(
                "Least squares requires explicit coordinates and a vector loss"
            )
        if not 0 < args.difference_step <= 0.1:
            raise ValueError("Difference step must be in (0, .1]")
    if args.plots and args.profile != "metal":
        raise ValueError("--plots currently supports only the metal profile")
    path = getattr(args, "bounds", None)
    if (args.stage == "coordinates") != (path is not None):
        raise ValueError("Explicit --bounds are required only for coordinates")
    args.coordinate_bounds = (
        json.loads(path.read_text(encoding="utf8")) if path else None
    )
    if path and (
        not isinstance(args.coordinate_bounds, dict) or not args.coordinate_bounds
    ):
        raise ValueError("Bounds must be a nonempty parameter mapping")
    return args
