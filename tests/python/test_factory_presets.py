"""Factory distribution must not contain local calibration attachments."""

import json
from pathlib import Path

import numpy as np
from drumfoundry import Renderer

ROOT = Path(__file__).resolve().parents[2]


def test_factory_presets_are_reference_free():
    paths = list((ROOT / "presets/factory").glob("*.fit.json"))
    assert len(paths) == 6
    for path in paths:
        document = json.loads(path.read_text(encoding="utf8"))
        assert set(document) == {
            "schema",
            "id",
            "name",
            "renderer",
            "instrument",
            "reference",
            "controls",
        }
        assert document["reference"] is None
        assert document["id"].startswith("factory.")
        assert "calibration" not in path.name


def test_official_hat_has_live_contact_and_audible_strike():
    path = ROOT / "presets/factory/hihat.fit.json"
    document = json.loads(path.read_text(encoding="utf8"))
    body = next(
        n["parameters"]
        for n in document["instrument"]["nodes"]
        if n["id"] == "rim-contact"
    )
    assert document["instrument"]["id"] == "factory.hihat"
    assert body["hat_contact_enabled"] == 1
    assert body["hat_rattle_motion"] > 0
    assert body["hat_settling"] > 0
    with Renderer(path, 48000) as voice:
        audio = voice.render(2)
        assert np.isfinite(audio).all()
        assert 0.01 < np.max(np.abs(audio)) < 1
