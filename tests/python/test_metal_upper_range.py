"""Extended metallic bandwidth retains old curves and explicit native state."""

from copy import deepcopy

import numpy as np
import pytest

from drumfoundry import Renderer, default_patch
from drumfoundry.fitting import parameters, with_parameters


def test_missing_upper_endpoint_preserves_legacy_render():
    explicit = with_parameters(default_patch(), {"body_decay_frequency_7": 15000})
    legacy = deepcopy(explicit)
    for node in legacy["nodes"]:
        node["parameters"].pop("body_decay_frequency_7", None)
    with Renderer(legacy) as old, Renderer(explicit) as new:
        assert parameters(old.document)["body_decay_frequency_7"] == 15000
        np.testing.assert_array_equal(old.render(0.15), new.render(0.15))


@pytest.mark.parametrize("rate", [32000, 44100, 48000, 96000])
def test_extended_centres_and_curve_roundtrip(rate):
    patch = with_parameters(
        default_patch(),
        {
            "resolved_frequency_0": 19500,
            "body_decay_frequency_7": 20000,
            "body_decay_active_1": 1,
            "body_decay_frequency_1": 17500,
            "body_decay_seconds_1": 2.5,
        },
    )
    with Renderer(patch, rate) as renderer:
        assert renderer.document == patch
        audio = renderer.render(0.15, [{"time": 0}, {"time": 0.07}])
        assert np.isfinite(audio).all()
        renderer.configure(renderer.document)
        np.testing.assert_array_equal(
            audio, renderer.render(0.15, [{"time": 0}, {"time": 0.07}])
        )


def test_active_knot_cannot_be_hidden_past_endpoint():
    patch = with_parameters(
        default_patch(),
        {
            "body_decay_active_1": 1,
            "body_decay_frequency_1": 17500,
        },
    )
    with pytest.raises(ValueError, match="upper endpoint"):
        Renderer(patch)


def test_kick_modal_limit_is_unchanged():
    with Renderer(default_patch("drum.kick.v1")) as renderer:
        frequency = next(
            p for p in renderer.descriptors if p["key"] == "resonance_frequency_0"
        )
        assert frequency["maximum"] == 15000
