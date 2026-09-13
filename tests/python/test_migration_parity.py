"""Permanent sound-preservation checks against the pre-extraction renderer."""

import json
from pathlib import Path

import numpy as np
import pytest

from drumfoundry.migration import render_case, signature, sound_identity

ROOT = Path(__file__).resolve().parents[2]
BASELINE = json.loads(
    (ROOT / "tests/fixtures/migration-v1.json").read_text(encoding="utf8")
)


def test_sound_identity_ignores_reference_but_not_strike_or_patch():
    document = json.loads((ROOT / "presets/kick_calibration.fit.json").read_text())
    identity = sound_identity(document)
    document["reference"] = None
    document["controls"]["analysis"]["size"] = 2048
    assert sound_identity(document) == identity
    document["controls"]["event"]["strength"] *= 0.5
    assert sound_identity(document) != identity
    event_identity = sound_identity(document)
    document["instrument"]["nodes"][0]["parameters"]["contact_level"] *= 0.5
    assert sound_identity(document) != event_identity


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
