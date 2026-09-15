"""Factory distribution must not contain local calibration attachments."""

import json
from pathlib import Path

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
