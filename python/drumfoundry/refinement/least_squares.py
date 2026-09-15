"""Bounded finite-difference fitting; steps are fractions of explicit UI ranges."""

import numpy as np
from scipy.optimize import least_squares


def fit(residual, start, *, iterations=20, step=0.005):
    """Probe every requested direction and run TRF with an absolute-step Jacobian.

    Unlike scipy's relative diff_step, this works at zero-valued controls.
    No automatic parameter pruning or undocumented influence threshold is used.
    The caller logs every native render, including probes and rejected trials.
    """
    if not 0 < step <= 0.1 or iterations < 1:
        raise ValueError(
            "Require a positive budget and finite difference step in (0, .1]"
        )
    start = np.asarray(start, dtype=float)
    cache = {}

    def evaluate(x):
        key = tuple(x)
        if key not in cache:
            value = np.asarray(residual(x), dtype=float)
            if value.ndim != 1 or not value.size or not np.isfinite(value).all():
                raise ValueError("Expected a finite nonempty residual vector")
            if len(cache) >= 128:
                cache.pop(next(iter(cache)))
            cache[key] = value
        return cache[key]

    def jacobian(x):
        columns = []
        for i in range(len(x)):
            minus, plus = x.copy(), x.copy()
            minus[i], plus[i] = max(0, x[i] - step), min(1, x[i] + step)
            columns.append((evaluate(plus) - evaluate(minus)) / (plus[i] - minus[i]))
        return np.column_stack(columns)

    influence = np.linalg.norm(jacobian(start), axis=0).tolist()
    result = least_squares(
        evaluate,
        start,
        jac=jacobian,
        bounds=(0, 1),
        method="trf",
        max_nfev=iterations,
        ftol=1e-4,
        xtol=1e-4,
        gtol=1e-4,
    )
    return dict(
        method="bounded-trf",
        difference_step=step,
        initial_residual_sensitivity=influence,
        nfev=result.nfev,
        status=int(result.status),
        message=result.message,
    )
