"""Known-answer checks for line-preserving percussion comparisons."""

import numpy as np
import pytest
from drumfoundry.refinement.tonal_layer_loss import TonalLayerLoss


def test_tonal_loss_identity_gain_and_noise_replacement():
    rate = 32000
    t = np.arange(3 * rate) / rate
    rng = np.random.default_rng(22)
    modal = sum(
        np.sin(2 * np.pi * f * t + p)
        for f, p in zip(np.geomspace(300, 12000, 70), rng.uniform(0, 6, 70))
    ) * np.exp(-4 * t)
    noise = rng.normal(size=len(t)) * np.exp(-4 * t)
    loss = TonalLayerLoss(modal, rate)
    assert loss.diagnostics(modal)["error_db"] == pytest.approx(0, abs=1e-10)
    assert loss.diagnostics(modal * 0.2)["error_db"] == pytest.approx(0, abs=1e-9)
    assert loss.components(noise)["ridge_contrast"] > 3
    assert np.linalg.norm(loss.residual(noise)) == pytest.approx(
        loss.diagnostics(noise)["error_db"]
    )
    assert loss.components(modal * np.exp(2 * t))["envelope"] > 4


def test_silent_tail_does_not_dilute_attack_error():
    rate = 32000
    t = np.arange(3 * rate) / rate
    noise = np.random.default_rng(4).normal(size=len(t))
    ref = noise * np.exp(-60 * t)
    loss = TonalLayerLoss(ref, rate)
    assert loss.active_blocks < len(loss.blocks)
    assert loss.components(noise * np.exp(-12 * t))["envelope"] > 5


@pytest.mark.parametrize("explicit_seeds", [False, True])
def test_parallel_native_layers_match_serial(explicit_seeds):
    from pathlib import Path
    from drumfoundry.refinement.saved import SavedFitRenderer
    from drumfoundry.refinement.layer_loss import LayerLoss
    from drumfoundry.refinement.layer_fit import LayerFit

    source = Path(__file__).resolve().parents[2] / "presets/factory/hihat.fit.json"
    with SavedFitRenderer(source, 16000) as saved:
        layers = []
        for i, strength in enumerate((0.25, 0.75)):
            event = dict(strength=strength)
            target = saved.render(saved.initial, 3, seed=1944, event=event)
            layers.append(dict(event=event, loss=LayerLoss(target, 16000)))
            if explicit_seeds:
                layers[-1]["training_seed"] = 100 + i
        p = saved.initial | {"model_level_db": -20}
        serial = LayerFit(saved, layers, p)
        parallel = LayerFit(saved, layers, p, workers=2)
        try:
            assert parallel.evaluate(p, "test") == pytest.approx(
                serial.evaluate(p, "test"), abs=1e-12
            )
            assert (
                parallel.rows[0]["layer_errors_db"] == serial.rows[0]["layer_errors_db"]
            )
            assert parallel.rows == serial.rows
            assert serial.rows[0]["layer_seeds"] == (
                [100, 101] if explicit_seeds else [1944, 1944]
            )
        finally:
            parallel.close()
            serial.close()


def test_low_prominence_does_not_also_boost_contact():
    from pathlib import Path
    from drumfoundry.refinement.saved import SavedFitRenderer
    from drumfoundry.refinement.fit_hat_body_ridge import prominence

    source = Path(__file__).resolve().parents[2] / "presets/factory/hihat.fit.json"
    with SavedFitRenderer(source, 44100) as saved:
        p = saved.initial | {"resolved_level_0": -12, "output_eq_enabled": 0}
        expected = saved.render(p | {"resolved_level_0": -6}, 0.4, seed=91)
        encoded = prominence(
            p, 6, p["resolved_frequency_0"], p["resolved_turbulence_0"]
        )
        actual = saved.render(encoded, 0.4, seed=91) * 10 ** (6 / 20)
        np.testing.assert_allclose(actual, expected, rtol=2e-5, atol=1e-6)
