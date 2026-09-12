"""Recover a known native parameter before trusting the fitting transport."""

from pathlib import Path
import numpy as np
import pytest

from drumfoundry import Renderer
from drumfoundry.fitting import fit_parameters, parameters, with_parameters

PRESET = Path(__file__).resolve().parents[2] / "presets/kick_calibration.fit.json"


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
