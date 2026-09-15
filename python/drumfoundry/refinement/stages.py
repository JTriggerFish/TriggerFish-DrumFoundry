"""Explicit search selection; historical per-ridge and EQ fitting is not automatic."""

import numpy as np
from scipy.optimize import minimize
from . import metal_search


def search(args, saved, rows, evaluate):
    p = saved.initial
    if getattr(args, "solver", "powell") == "least-squares" and (
        args.stage != "coordinates" or args.profile == "metal"
    ):
        raise ValueError(
            "Least squares requires explicit coordinates and a vector loss"
        )
    if args.stage == "audit-only" or not args.budget:
        return
    if args.stage == "coordinates":
        if args.bounds is None:
            raise ValueError(
                "Coordinate search requires an explicit --bounds JSON file"
            )
        from drumfoundry.fitting import _coordinates

        bounds = args.coordinate_bounds
        keys, low, high, start = _coordinates(saved.renderer, saved.fit, bounds)
        if getattr(args, "solver", "powell") == "least-squares":
            from .least_squares import fit

            result = fit(
                lambda x: evaluate(
                    dict(p, **dict(zip(keys, (low + x * (high - low)).tolist()))),
                    "explicit coordinates",
                    residual=True,
                ),
                start,
                iterations=args.budget,
                step=args.difference_step,
            )
            return dict(result, parameters=keys)
        minimize(
            lambda x: evaluate(
                dict(p, **dict(zip(keys, (low + x * (high - low)).tolist()))),
                "explicit coordinates",
            ),
            start,
            method="Powell",
            bounds=[(0, 1)] * len(keys),
            options=dict(maxfev=args.budget, xtol=0.01, ftol=0.002),
        )
        return
    if args.profile != "metal":
        if args.profile == "gong-onset" and args.stage == "gong-grid":
            from itertools import product

            for tilt, decay in product((-30, -28, -26), (1.25, 1.5, 1.75)):
                evaluate(
                    dict(p, body_brightness=tilt, body_decay_seconds_7=decay),
                    f"excitation {tilt}, upper T60 {decay}",
                )
            return
        raise ValueError("This profile requires coordinates, gong-grid or audit-only")
    if saved.fit["instrument"]["recipe"] != "metal.cymbal.v1":
        raise ValueError("Structured metal stages require a metallic-plate patch")
    if args.stage == "locked-texture":
        metal_search.fit_locked_texture(p, evaluate, args.budget)
    elif args.stage == "shared-series":
        metal_search.fit_shared_series(p, rows, evaluate, args.budget)
    elif args.stage in ("sparse-decay", "fine-decay"):
        metal_search.fit_sparse_decay(p, rows, evaluate, args.stage == "fine-decay")
    elif args.stage == "upper-balance":
        metal_search.fit_upper_balance(p, evaluate)
    elif args.stage == "low-decay":
        for scale in (0.65, 0.8, 0.9):
            evaluate(
                dict(
                    p, body_decay_seconds_0=max(0.02, p["body_decay_seconds_0"] * scale)
                ),
                f"low T60 multiplier {scale}",
            )
    else:
        raise ValueError("Unsupported profile/stage combination")
