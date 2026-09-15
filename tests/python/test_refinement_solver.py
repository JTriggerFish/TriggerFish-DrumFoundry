"""Recover a known bounded target, including a control initially at zero."""

import numpy as np
from drumfoundry.refinement.least_squares import fit


def test_absolute_steps_at_zero_and_influence():
    seen = []
    target = np.array([0.35, 0.8])

    def residual(x):
        seen.append(x.copy())
        return x - target

    report = fit(residual, [0, 0.5], iterations=30)
    assert min(np.linalg.norm(x - target) for x in seen) < 1e-3
    np.testing.assert_allclose(report["initial_residual_sensitivity"], [1, 1])
    assert all(np.all((x >= 0) & (x <= 1)) for x in seen)
