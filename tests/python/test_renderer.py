"""Native preset fidelity, error boundaries and independent instance ownership."""

from copy import deepcopy
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import numpy as np
import pytest

from drumfoundry import Renderer, default_patch

ROOT = Path(__file__).resolve().parents[2]
PRESETS = sorted((ROOT / "presets").glob("*.json"))


@pytest.mark.parametrize("path", PRESETS, ids=lambda p: p.stem)
def test_presets_roundtrip_and_repeat_deterministically(path):
    import json

    original = json.loads(path.read_text(encoding="utf8"))
    with Renderer(path) as r:
        assert r.document == original
        a, b = r.render(0.2), r.render(0.2)
        np.testing.assert_array_equal(a, b)
        assert np.isfinite(a).all() and np.max(np.abs(a)) > 1e-4
        assert not np.any(r.render(0.2, []))
        assert len(r.descriptors) > 0


@pytest.mark.parametrize(
    "recipe", ["metal.cymbal.v1", "drum.kick.v1", "drum.membrane.v1", "drum.snare.v1"]
)
def test_native_default_patches_are_complete(recipe):
    patch = default_patch(recipe)
    with Renderer(patch) as r:
        assert sum(len(n["parameters"]) for n in patch["nodes"]) == len(r.descriptors)
        assert np.max(np.abs(r.render(0.1))) > 0


def test_block_sizes_and_restrikes_preserve_state():
    with Renderer(PRESETS[0]) as r:
        expected = r.render(0.2, [{"time": 0}, {"time": 0.1}])
        r.reset()
        pieces = []
        for _ in range(2):
            r.trigger()
            for size in [128] * 37 + [64]:
                pieces.append(r.process(size))
        np.testing.assert_array_equal(np.concatenate(pieces), expected)


def test_more_than_four_instances_and_parallel_rendering():
    def run(_):
        with Renderer(PRESETS[0]) as r:
            return r.render(0.1)

    instances = [Renderer(PRESETS[0]) for _ in range(12)]
    try:
        assert all(np.max(np.abs(r.render(0.02))) > 0 for r in instances)
        with ThreadPoolExecutor(max_workers=6) as pool:
            results = list(pool.map(run, range(12)))
        for result in results[1:]:
            np.testing.assert_array_equal(result, results[0])
    finally:
        for renderer in instances:
            renderer.close()


@pytest.mark.parametrize(
    "damage", ["unknown", "owner", "route", "duplicate", "version", "event", "gain"]
)
def test_bad_patch_is_rejected_transactionally(damage):
    with Renderer(PRESETS[0]) as r:
        expected = r.render(0.05)
        document = r.document
        patch = document["instrument"]
        if damage == "unknown":
            patch["nodes"][0]["parameters"]["not_a_control"] = 1
        elif damage == "owner":
            patch["nodes"][0]["parameters"]["model_level_db"] = -6
        elif damage == "route":
            patch["connections"][-1]["enabled"] = False
        elif damage == "duplicate":
            patch["nodes"][1]["id"] = patch["nodes"][0]["id"]
        elif damage == "version":
            patch["nodes"][0]["version"] = 2
        elif damage == "event":
            document["controls"]["event"]["seed"] = -1
        elif damage == "gain":
            patch["connections"][0]["gain"] = 0.5
        with pytest.raises(ValueError):
            r.configure(document)
        np.testing.assert_array_equal(r.render(0.05), expected)


def test_duplicate_json_keys_not_swallowed():
    with pytest.raises(ValueError, match="Duplicate JSON key"):
        Renderer('{"schema": "a", "schema": "b"}')


@pytest.mark.parametrize(
    "event",
    [
        {"seed": -1},
        {"seed": 1.5},
        {"seed": 2**32},
        {"strength": float("nan")},
        {"strength": 1.01},
        {"typo": 0},
    ],
)
def test_invalid_strikes(event):
    with Renderer(PRESETS[0]) as r, pytest.raises(ValueError):
        r.trigger(**event)


def test_closed_renderer_is_safe():
    r = Renderer(PRESETS[0])
    r.close()
    r.close()
    with pytest.raises(RuntimeError, match="closed"):
        r.render(0.1)


def test_zero_output_is_not_auto_normalized():
    patch = default_patch("metal.cymbal.v1")
    patch["nodes"][1]["parameters"]["field_gain"] = 0  # wrong owner must fail
    with pytest.raises(ValueError):
        Renderer(patch)
    del patch["nodes"][1]["parameters"]["field_gain"]
    patch["nodes"][2]["parameters"].update(field_gain=0, direct_gain=0)
    with Renderer(patch) as r:
        assert np.count_nonzero(r.render(0.1)) == 0
