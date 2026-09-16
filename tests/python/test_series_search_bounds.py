"""Native frequency-range constraints reject trials, not whole fitting runs."""

import numpy as np
import pytest
from drumfoundry.refinement import metal_search
from test_metal_refinement import parameters


@pytest.mark.parametrize("locked", [False, True])
def test_optimizer_rejects_overflow_then_continues(monkeypatch, locked):
    base = parameters()
    for i in range(32):
        base[f"resolved_level_{i}"] = -72
    for i, frequency in enumerate([100, 200, 300, 18000, 19999]):
        base[f"resolved_level_{i}"] = -6
        base[f"resolved_frequency_{i}"] = frequency
    rows = []

    def evaluate(p, label):
        assert all(p[f"resolved_frequency_{i}"] < 20000 for i in range(5))
        rows.append(dict(parameters=p, score=1))
        return 1

    def minimize(score, start, **kwargs):
        before = len(rows)
        assert np.isinf(score(np.ones(len(start))))
        assert len(rows) == before  # No render, loss, or invalid trace row.
        assert score(np.zeros(len(start))) == 1
        assert len(rows) == before + 1

    monkeypatch.setattr(metal_search, "minimize", minimize)
    if locked:
        metal_search.fit_locked_texture(base, evaluate, 10)
    else:
        metal_search.fit_shared_series(base, rows, evaluate, 10)


def test_unexpected_edit_failure_is_not_silently_rejected():
    def broken(*args):
        raise ValueError("invalid source")

    with pytest.raises(ValueError, match="invalid source"):
        metal_search._feasible_score(broken, {}, [], None, "test")
