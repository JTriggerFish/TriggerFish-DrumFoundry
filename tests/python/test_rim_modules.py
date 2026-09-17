"""Optional module ownership and the native Python live-control boundary."""

from copy import deepcopy
import json
from pathlib import Path

import numpy as np
import pytest

from drumfoundry import Renderer
from drumfoundry.migration import sound_identity
from drumfoundry.renderer import default_patch


@pytest.mark.parametrize(
    "recipe", ["metal.cymbal.v1", "drum.kick.v1", "drum.membrane.v1", "drum.snare.v1"]
)
def test_optional_contact(recipe):
    patch = default_patch(recipe)
    bare = deepcopy(patch)
    bare["nodes"] = [n for n in bare["nodes"] if n["id"] != "rim-contact"]
    bare.pop("attachments")
    with Renderer(patch) as attached, Renderer(bare) as absent:
        assert all(d["owner"] != "rim-contact" for d in absent.descriptors)
        np.testing.assert_array_equal(attached.render(0.2), absent.render(0.2))
        with pytest.raises(ValueError, match="Unknown parameter"):
            absent.set_parameter("hat_contact_enabled", 1)
        attached.set_parameter("hat_contact_enabled", 1)
        attached.reset()
        assert not np.any(attached.process(512))
        attached.set_parameter("hat_openness", 0)
        samples = attached.process(4096)
        assert np.isfinite(samples).all() and np.max(np.abs(samples)) > 1e-8
        attached.reset()
        assert not np.any(attached.process(512))


def test_legacy_ownership_upgrades_without_changing_identity():
    legacy = Path(__file__).resolve().parents[2] / "presets/legacy/hihat.fit.json"
    old = json.loads(legacy.read_text())
    with Renderer(old) as voice:
        modern = voice.document
    assert sound_identity(old) == sound_identity(modern)
    module = next(n for n in modern["instrument"]["nodes"] if n["id"] == "rim-contact")
    module["parameters"]["hat_contact_enabled"] = 1
    assert sound_identity(modern) != sound_identity(old)
