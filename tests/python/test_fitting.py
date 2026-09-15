"""Recover a known native parameter before trusting the fitting transport."""

from pathlib import Path
import numpy as np
import pytest

from drumfoundry import Renderer, default_patch
from drumfoundry.fitting import fit_parameters, parameters, with_parameters

PRESET = Path(__file__).resolve().parents[2] / "presets/factory/kick.fit.json"


def test_recover_known_gain_without_changing_other_controls():
    with Renderer(PRESET) as renderer:
        original = renderer.document
        target = with_parameters(original, {"model_level_db": -14})
        renderer.configure(target)
        reference = renderer.render(0.15)
        renderer.configure(with_parameters(original, {"model_level_db": -22}))
        before = renderer.document
        candidate, report = fit_parameters(
            renderer,
            reference,
            {"model_level_db": (-30, -5)},
            lambda audio: audio - reference,
            budget=45,
        )
        assert abs(parameters(candidate)["model_level_db"] + 14) < 0.03
        assert report["best_score"] < report["baseline_score"] * 1e-4
        assert len(report["trace"]) <= 45
        assert renderer.document == before
        unchanged = parameters(before)
        unchanged["model_level_db"] = parameters(candidate)["model_level_db"]
        assert parameters(candidate) == unchanged


def test_objective_failure_restores_renderer():
    with Renderer(PRESET) as renderer:
        before = renderer.document
        with pytest.raises(ValueError, match="non-finite"):
            fit_parameters(
                renderer,
                renderer.render(0.1),
                {"model_level_db": (-60, 0)},
                lambda _: np.nan,
                budget=2,
            )
        assert renderer.document == before


def test_discrete_coordinate_rejected_before_rendering():
    with Renderer(default_patch()) as renderer:
        before = renderer.document
        with pytest.raises(ValueError, match="continuous search coordinate"):
            fit_parameters(
                renderer,
                np.zeros(100),
                {"field_distribution": (0, 4)},
                lambda _: pytest.fail("Discrete coordinate reached objective"),
            )
        assert renderer.document == before


def test_decimal_search_endpoints_match_native_validation():
    with Renderer(default_patch("drum.kick.v1")) as renderer:
        reference = renderer.render(0.1)
        _, report = fit_parameters(
            renderer,
            reference,
            {"contact_width_seconds": (0.0002, 0.08)},
            lambda audio: audio - reference,
            budget=2,
        )
        assert len(report["trace"]) == 2


def test_float_endpoint_start_is_inside_decimal_bounds():
    patch = with_parameters(
        default_patch("drum.kick.v1"),
        {"contact_width_seconds": float(np.float32(0.0002))},
    )
    with Renderer(patch) as renderer:
        _, report = fit_parameters(
            renderer,
            renderer.render(0.1),
            {"contact_width_seconds": (0.0002, 0.08)},
            lambda _: 0.0,
            budget=1,
        )
        assert report["best_score"] == 0
