"""Known-answer onset/bloom checks precede fitting real metallic instruments."""

import numpy as np
import pytest
from drumfoundry.refinement.temporal_metal_loss import TemporalMetalLoss


@pytest.mark.parametrize("rate", [32000, 44100, 48000])
def test_upper_band_is_scored_without_crossing_nyquist(rate):
    t = np.arange(3 * rate) / rate
    upper = min(20000, 0.48 * rate)
    low = np.sin(2 * np.pi * 470 * t) * np.exp(-4 * t)
    high = np.sin(2 * np.pi * (upper - 300) * t) * np.exp(-4 * t)
    reference = low + high
    loss = TemporalMetalLoss(reference, rate)
    assert loss.specification["bands_hz"][-1][1] == upper
    assert loss.specification["spectrum_hz"][-1] == upper
    assert loss.specification["texture_bands_hz"][-1][1] == upper
    assert np.linalg.norm(loss.residual(reference)) < 1e-10
    # Equal total energy, but the upper ridge is missing/replaced by low energy.
    wrong = loss.diagnostics(low * np.sqrt(2))
    assert wrong["spectrum"] > 1
    assert wrong["onset"] > 1
    assert np.isfinite(loss.residual(low)).all()


def test_tone_then_sizzle_and_gain():
    rate = 32000
    t = np.arange(3 * rate) / rate
    low = np.sin(2 * np.pi * 470 * t) * np.exp(-t / 0.25)
    upper = sum(np.sin(2 * np.pi * f * t) for f in (5200, 6437, 8191, 10003)) / 4
    envelope = (1 - np.exp(-t / 0.06)) * np.exp(-t / 0.22)
    reference = low + upper * envelope
    loss = TemporalMetalLoss(reference, rate)
    assert np.linalg.norm(loss.residual(reference)) < 1e-10
    assert np.linalg.norm(loss.residual(reference * 0.1)) < 1e-10
    premature = low + upper * np.exp(-t / 0.22)
    wrong = loss.diagnostics(premature)
    assert wrong["rise"] > 2
    assert wrong["onset"] > 1
    slow = loss.diagnostics(low + upper * (1 - np.exp(-t / 0.06)) * np.exp(-t / 0.6))
    assert slow["decay"] > wrong["decay"]
    np.testing.assert_allclose(
        np.linalg.norm(loss.residual(premature)), wrong["error_db"]
    )
    # Extra energy after the reference has ended cannot disappear from the
    # objective merely because onset and bloom already match.
    lingering = reference.copy()
    lingering[t > 1.5] += 0.04 * np.sin(2 * np.pi * 6437 * t[t > 1.5])
    assert loss.diagnostics(lingering)["tail"] > 5
