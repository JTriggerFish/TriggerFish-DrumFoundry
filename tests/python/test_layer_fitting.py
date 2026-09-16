"""Known answers for multi-layer ranking and restricted modal coordinates."""

import numpy as np
import pytest
from drumfoundry.refinement.layer_loss import LayerLoss
from drumfoundry.refinement.layer_series import stretched_series
from drumfoundry.refinement.layer_fit import LayerFit


def test_layer_loss_identity_level_and_decay():
    rate = 16000
    t = np.arange(3 * rate) / rate
    noise = np.random.default_rng(40).normal(size=len(t))
    target = noise * np.exp(-t * 8)
    loss = LayerLoss(target, rate)
    assert np.linalg.norm(loss.residual(target)) == 0
    assert np.linalg.norm(loss.residual(target * 0.5)) > 1
    assert np.linalg.norm(loss.residual(noise * np.exp(-t * 3))) > 3
    assert (
        np.linalg.norm(loss.residual(np.sin(2 * np.pi * 3000 * t) * np.exp(-8 * t))) > 3
    )


def test_timbre_policy_removes_only_global_level_not_decay():
    rate = 16000
    t = np.arange(3 * rate) / rate
    noise = np.random.default_rng(64).normal(size=len(t))
    target = noise * np.exp(-8 * t)
    loss = LayerLoss(target, rate, "timbre")
    quieter = target * 0.125
    copy = quieter.copy()
    assert loss.diagnostics(quieter)["error_db"] < 1e-10
    assert loss.diagnostics(quieter)["comparison_gain_db"] == pytest.approx(18.0617997)
    assert loss.diagnostics(noise * np.exp(-3 * t))["error_db"] > 3
    np.testing.assert_array_equal(quieter, copy)


def test_series_preserves_core_and_never_fits_individual_levels():
    p = stretched_series({"direct_gain": 0.1}, 250, 0.2)
    assert [p[f"resolved_frequency_{i}"] for i in range(3)] == [250, 500, 750]
    assert p["resolved_frequency_23"] > 6000
    assert len({p[f"resolved_level_{i}"] for i in range(24)}) == 1
    assert p["resolved_level_24"] == -72
    assert p["direct_gain"] == 0.1
    with pytest.raises(ValueError):
        stretched_series({}, 1000, 1)


def test_series_supports_native_upper_range():
    p = stretched_series({}, 1000, 0, count=20)
    assert p["resolved_frequency_19"] == 20000
    with pytest.raises(ValueError):
        stretched_series({}, 1000, 0, count=21)


def test_joint_layers_preserve_velocity_and_receive_equal_weight():
    class Voice:
        def __init__(self):
            self.hits = []

        def render(self, parameters, seconds, seed, event):
            self.hits.append(dict(event))
            return np.array([parameters["gain"] * event["strength"]])

    class Loss:
        def __init__(self, expected):
            self.expected = expected

        def diagnostics(self, audio):
            return dict(error_db=abs(float(audio[0]) - self.expected))

    voice = Voice()
    layers = [
        dict(event=dict(strength=0.25), loss=Loss(0.5)),
        dict(event=dict(strength=0.9), loss=Loss(1.8)),
    ]
    fit = LayerFit(voice, layers, {"gain": 1})
    assert fit.evaluate({"gain": 1}, "before") == pytest.approx(
        np.hypot(0.25, 0.9) / np.sqrt(2)
    )
    assert fit.evaluate({"gain": 2}, "known answer") == 0
    assert fit.best == {"gain": 2}
    assert voice.hits == [dict(strength=0.25), dict(strength=0.9)] * 2
    fit.evaluate({"gain": 9}, "audit", accept=False)
    assert fit.best == {"gain": 2}


def test_layer_comparison_rejects_wrong_length_and_nonfinite_audio():
    target = np.ones(48000)
    loss = LayerLoss(target, 16000)
    with pytest.raises(ValueError):
        loss.residual(target[:-1])
    with pytest.raises(ValueError):
        loss.residual(target * np.nan)


def test_publish_three_states_with_identical_velocity_views(tmp_path):
    from pathlib import Path
    from hashlib import sha256
    from drumfoundry.refinement.artifacts import write_json
    from drumfoundry.refinement.publish_hat_audition import publish
    from drumfoundry import Renderer

    source = Path(__file__).resolve().parents[2] / "presets/factory/hihat.fit.json"
    with Renderer(source) as voice:
        fit = voice.document
    run = tmp_path / "run"
    for state in ("closed", "half-open", "open"):
        folder = run / state
        folder.mkdir(parents=True)
        rows = []
        for i in range(4):
            path = folder / f"{i}.fit.json"
            write_json(path, fit)
            rows.append(dict(fit_sha256=sha256(path.read_bytes()).hexdigest()))
        write_json(folder / "report.json", dict(exact_reload=True, layers=rows))
    destination = tmp_path / "auditions"
    paths = publish(run, destination)
    assert len(paths) == 15
    assert len(list(destination.glob("*.fit.json"))) == 3
    with pytest.raises(FileExistsError):
        publish(run, destination)
    (run / "open" / "0.fit.json").write_text("{}", encoding="utf8")
    with pytest.raises(ValueError, match="changed since"):
        publish(run, tmp_path / "bad")
    assert not (tmp_path / "bad").exists()


def test_recover_one_native_gain_across_three_velocities():
    from pathlib import Path
    from drumfoundry.refinement.saved import SavedFitRenderer

    source = Path(__file__).resolve().parents[2] / "presets/factory/kick.fit.json"
    with SavedFitRenderer(source, 16000) as saved:
        target = saved.initial | {"model_level_db": -14}
        layers = []
        for strength in (0.25, 0.6, 0.9):
            event = dict(strength=strength)
            reference = saved.render(target, 3, seed=1944, event=event)
            layers.append(dict(event=event, loss=LayerLoss(reference, 16000)))
        start = target | {"model_level_db": -24}
        fit = LayerFit(saved, layers, start)
        fit.stage({"model_level_db": (-30, -5, False)}, 40, "known gain")
        assert abs(fit.best["model_level_db"] + 14) < 0.04
        assert fit.score < 0.04
        assert {k: v for k, v in fit.best.items() if k != "model_level_db"} == {
            k: v for k, v in start.items() if k != "model_level_db"
        }


def test_observation_headroom_does_not_change_synthesis():
    from pathlib import Path
    from drumfoundry.refinement.saved import SavedFitRenderer
    from drumfoundry.refinement.layer_series import observation_headroom

    source = Path(__file__).resolve().parents[2] / "presets/factory/hihat.fit.json"
    with SavedFitRenderer(source, 44100) as saved:
        p = stretched_series(saved.initial, 260, 0.22)
        a = saved.render(p, 0.3, seed=1944)
        q = observation_headroom(p)
        b = saved.render(q, 0.3, seed=1944)
        np.testing.assert_allclose(a, b, rtol=2e-5, atol=1e-6)
        assert q["model_level_db"] == p["model_level_db"] - 12
        assert q["body_excitation"] == p["body_excitation"]
