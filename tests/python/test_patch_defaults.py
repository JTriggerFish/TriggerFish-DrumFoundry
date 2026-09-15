"""Partial patches expand into explicit, reproducible native state."""

from copy import deepcopy
from pathlib import Path

import numpy as np
import pytest

from drumfoundry import Renderer, default_patch
from drumfoundry.fitting import parameters, with_parameters


@pytest.mark.parametrize(
    "recipe", ["metal.cymbal.v1", "drum.kick.v1", "drum.membrane.v1", "drum.snare.v1"]
)
def test_partial_patch_expands_and_roundtrips(recipe):
    full = with_parameters(default_patch(recipe), {"model_level_db": -17.25})
    partial = deepcopy(full)
    for node in partial["nodes"]:
        if "model_level_db" in node["parameters"]:
            node["parameters"] = {"model_level_db": -17.25}
        else:
            del node["parameters"]
    with Renderer(partial) as r:
        assert r.document == full
        assert len(parameters(r.document)) == len(r.descriptors)
        actual = r.render(0.2)
        r.configure(r.document)
        np.testing.assert_array_equal(actual, r.render(0.2))
    with Renderer(full) as r:
        np.testing.assert_array_equal(actual, r.render(0.2))


@pytest.mark.parametrize("invalid", [None, [], 0])
def test_present_invalid_parameters_are_not_defaulted(invalid):
    patch = default_patch()
    patch["nodes"][0]["parameters"] = invalid
    with pytest.raises(ValueError, match="parameters object"):
        Renderer(patch)


@pytest.mark.parametrize(
    "key,value", [("contact_width_seconds", 0.08), ("thump_pitch_fall_seconds", 0.003)]
)
def test_decimal_endpoints_are_preserved(key, value):
    patch = with_parameters(default_patch("drum.kick.v1"), {key: value})
    with Renderer(patch) as r:
        assert r.document == patch


@pytest.mark.parametrize("value", [0.08001, 0.00019, 1e300])
def test_outside_endpoint_still_rejected(value):
    patch = with_parameters(
        default_patch("drum.kick.v1"), {"contact_width_seconds": value}
    )
    with pytest.raises(ValueError, match="Invalid parameter"):
        Renderer(patch)


def test_ring_character_is_discrete():
    with Renderer(default_patch()) as r:
        d = next(d for d in r.descriptors if d["key"] == "field_distribution")
        assert d["scale"] == 3
        for choice in range(5):
            r.configure(with_parameters(r.document, {"field_distribution": choice}))
        for invalid in (1.2, 1.0000000001, -1, 5):
            with pytest.raises(ValueError, match="field_distribution"):
                r.configure(
                    with_parameters(r.document, {"field_distribution": invalid})
                )


def test_fit_metadata_and_overrides_survive_expansion():
    preset = Path(__file__).resolve().parents[2] / "presets/factory/crash.fit.json"
    with Renderer(preset) as r:
        full = r.document
        partial = deepcopy(full)
        # Delete one parameter whose value is explicitly its native default.
        descriptor = next(
            d for d in r.descriptors if parameters(full)[d["key"]] == d["default"]
        )
        for node in partial["instrument"]["nodes"]:
            node["parameters"].pop(descriptor["key"], None)
        r.configure(partial)
        assert r.document == full
        assert parameters(partial).get(descriptor["key"]) is None
