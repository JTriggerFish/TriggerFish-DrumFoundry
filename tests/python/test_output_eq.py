"""Every instrument exposes one final EQ, including older saved drum fits."""

import copy

import numpy as np
import pytest

from drumfoundry import Renderer
from drumfoundry.renderer import default_patch

KEYS = {
    "output_eq_enabled",
    "output_low_cut",
    "output_high_cut",
    "output_colour_frequency",
    "output_colour_gain",
}
OLD_NAMES = {
    "output_eq_enabled": "equalizer_mode",
    "output_low_cut": "low_cut_hz",
    "output_high_cut": "high_cut_hz",
    "output_colour_frequency": "colour_frequency_hz",
    "output_colour_gain": "colour_gain_db",
}


@pytest.mark.parametrize(
    "recipe", ["metal.cymbal.v1", "drum.kick.v1", "drum.membrane.v1", "drum.snare.v1"]
)
def test_single_eq_owner(recipe):
    with Renderer(default_patch(recipe)) as voice:
        eq = [p for p in voice.descriptors if p["key"] in KEYS]
        assert len(eq) == 5
        assert len({p["owner"] for p in eq}) == 1
        assert not any(
            p["key"].startswith("band_") or p["key"] == "equalizer_mode"
            for p in voice.descriptors
        )


@pytest.mark.parametrize(
    "recipe", ["drum.kick.v1", "drum.membrane.v1", "drum.snare.v1"]
)
def test_old_eq_converts_without_retaining_dead_controls(recipe):
    patch = default_patch(recipe)
    old = copy.deepcopy(patch)
    params = next(
        n["parameters"] for n in old["nodes"] if "output_eq_enabled" in n["parameters"]
    )
    for new, legacy in OLD_NAMES.items():
        params[legacy] = params.pop(new)
    params["band_1_gain_db"] = 6  # Inactive, not a hidden coefficient.
    params["band_1_frequency_hz"] = 100
    with Renderer(old) as converted, Renderer(patch) as current:
        assert converted.document == current.document
        np.testing.assert_array_equal(converted.render(0.1), current.render(0.1))
    params["equalizer_mode"] = 2
    with pytest.raises(ValueError, match="retired multiband EQ"):
        Renderer(old)
