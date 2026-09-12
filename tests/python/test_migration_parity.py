"""Permanent sound-preservation checks against the pre-extraction renderer."""

import json
from pathlib import Path

import numpy as np
import pytest

from drumfoundry.migration import render_case, signature

ROOT = Path(__file__).resolve().parents[2]
BASELINE = json.loads(
    (ROOT / "tests/fixtures/migration-v1.json").read_text(encoding="utf8")
)


@pytest.mark.parametrize(
    "case",
    BASELINE["cases"],
    ids=lambda c: f"{c['preset']}-{c['rate']}-{c['repeated']}",
)
def test_legacy_temporal_and_spectral_signature(case):
    actual = signature(render_case(ROOT / "presets", case), case["rate"])
    for key in ("temporal_rms", "spectral_rms"):
        expected = np.array(case[key])
        np.testing.assert_allclose(
            actual[key], expected, rtol=0.005, atol=max(expected) * 1e-5
        )
