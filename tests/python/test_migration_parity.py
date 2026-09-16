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
    document = json.loads((ROOT / "presets/factory/kick.fit.json").read_text())
    identity = sound_identity(document)
    document["reference"] = None
    document["controls"]["analysis"]["size"] = 2048
    assert sound_identity(document) == identity
    document["controls"]["event"]["strength"] *= 0.5
    assert sound_identity(document) != identity
    event_identity = sound_identity(document)
    document["instrument"]["nodes"][0]["parameters"]["contact_level"] *= 0.5
    assert sound_identity(document) != event_identity


def test_explicit_legacy_q_retains_oracle_identity():
    document = json.loads((ROOT / "presets/factory/gong.fit.json").read_text())
    eq = next(
        n["parameters"]
        for n in document["instrument"]["nodes"]
        if "output_colour_q" in n["parameters"]
    )
    explicit = sound_identity(document)
    assert eq.pop("output_colour_q") == 0.7
    assert sound_identity(document) == explicit
    eq["output_colour_q"] = 0.8
    assert sound_identity(document) != explicit


def test_explicit_legacy_endpoint_retains_oracle_identity():
    document = json.loads((ROOT / "presets/factory/hihat.fit.json").read_text())
    body = next(
        n["parameters"] for n in document["instrument"]["nodes"] if n["id"] == "body"
    )
    explicit = sound_identity(document)
    assert body.pop("body_decay_frequency_7") == 15000
    assert sound_identity(document) == explicit
    body["body_decay_frequency_7"] = 20000
    assert sound_identity(document) != explicit


@pytest.mark.parametrize(
    "case",
    BASELINE["cases"],
    ids=lambda c: f"{c['preset']}-{c['rate']}-{c['repeated']}",
)
def test_legacy_temporal_and_spectral_signature(case):
    actual = signature(render_case(ROOT / "presets/factory", case), case["rate"])
    for key in ("temporal_rms", "spectral_rms"):
        expected = np.array(case[key])
        np.testing.assert_allclose(
            actual[key], expected, rtol=0.005, atol=max(expected) * 1e-5
        )
