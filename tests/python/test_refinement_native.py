"""No browser, sample server or private corpus is needed for fit/audit contracts."""

import json
from pathlib import Path
from types import SimpleNamespace
import numpy as np
import pytest
from scipy.io import wavfile

from drumfoundry import Renderer
from drumfoundry.refinement.saved import SavedFitRenderer
from drumfoundry.refinement.reference import load_reference
from drumfoundry.refinement.run import run
from drumfoundry.refinement.audit import audit

PRESET = Path(__file__).resolve().parents[2] / "presets/factory/kick.fit.json"


def test_native_saved_render_preserves_routing_gesture_and_sequence():
    with SavedFitRenderer(json.loads(PRESET.read_text())) as saved:
        fit = saved.snapshot(saved.initial)
        fit["controls"]["event"]["strength"] = 0.37
        fit["instrument"]["connections"][0]["enabled"] = False
        with SavedFitRenderer(fit) as changed, Renderer(fit) as direct:
            np.testing.assert_array_equal(
                changed.render(changed.initial, 0.4), direct.render(0.4)
            )
            hits = [dict(time=0, strength=0.8), dict(time=0.13, strength=0.2)]
            np.testing.assert_array_equal(
                changed.sequence(changed.initial, 0.4, hits), direct.render(0.4, hits)
            )
            assert (
                changed.snapshot(changed.initial)["controls"]["event"]
                == fit["controls"]["event"]
            )
            with pytest.raises(ValueError, match="surface"):
                changed.snapshot({})


def test_reference_gain_channel_alignment_and_hash(tmp_path):
    path = tmp_path / "reference.wav"
    audio = np.column_stack((np.arange(1000) / 1000, -np.arange(1000) / 2000)).astype(
        "float32"
    )
    wavfile.write(path, 16000, audio)
    ref = load_reference({}, path, 16000, gain_db=6, onset=0.01, channel=1)
    np.testing.assert_allclose(ref.samples, audio[160:, 0] * 10 ** (6 / 20), rtol=1e-6)
    assert len(ref.window(0.1)) == 1600
    with pytest.raises(ValueError, match="hash"):
        load_reference({"reference": {"sha256": "incorrect"}}, path, 16000)
    with pytest.raises(ValueError, match="outside"):
        load_reference({}, path, 16000, onset=1)


def test_native_fit_and_independent_audit(tmp_path):
    fit = tmp_path / "source.json"
    fit.write_bytes(PRESET.read_bytes())
    reference = tmp_path / "reference.wav"
    with Renderer(PRESET, 16000) as renderer:
        wavfile.write(reference, 16000, renderer.render(1.2))
    args = SimpleNamespace(
        fit=fit,
        reference=reference,
        library_root=tmp_path,
        rate=16000,
        seconds=1.2,
        gain=0,
        onset=0,
        channel=0,
        output=tmp_path / "fit",
        profile="kick",
        stage="audit-only",
        budget=0,
        name="Test",
        plots=False,
    )
    candidate, report = run(args)
    assert report["selected"]["components"][0]["score"] < 1e-6
    assert (
        candidate["controls"]["event"]
        == json.loads(PRESET.read_text())["controls"]["event"]
    )
    result = audit(args.output, reference, tmp_path / "audit")
    assert result["candidate"]["exact_reload"]
    assert not set(result["audit_seeds"]) & set(result["training_seeds"])
    assert len(result["performance"]) == 5
    with pytest.raises(ValueError, match="fresh --output"):
        run(args)
    candidate["name"] = "Changed after fitting"
    (args.output / "candidate.fit.json").write_text(json.dumps(candidate))
    with pytest.raises(ValueError, match="Frozen fit changed"):
        audit(args.output, reference, tmp_path / "bad-audit")

    args.output = tmp_path / "coordinates"
    args.stage, args.solver, args.budget = "coordinates", "least-squares", 2
    args.difference_step = 0.005
    args.bounds = tmp_path / "bounds.json"
    args.bounds.write_text(json.dumps({"model_level_db": [-18, -6]}))
    _, trace = run(args)
    assert trace["selected"]["score"] <= trace["rows"][0]["score"]
    assert trace["solver_report"]["initial_residual_sensitivity"][0] > 0
