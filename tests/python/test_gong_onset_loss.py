"""Causal onset measurements retain level and penalize premature upper bloom."""

import numpy as np
from drumfoundry.refinement.gong_onset import GongOnsetLoss


def test_identity_level_and_early_high_energy():
    rate = 32000
    t = np.arange(3 * rate) / rate
    audio = 0.1 * np.random.default_rng(11).normal(size=len(t)) * np.exp(-t)
    loss = GongOnsetLoss(audio, rate)
    assert np.linalg.norm(loss.residual(audio)) == 0
    np.testing.assert_allclose(
        np.linalg.norm(loss.residual(audio * 0.5)), 20 * np.log10(2), rtol=1e-6
    )
    altered = audio + 0.15 * np.sin(2 * np.pi * 6000 * t) * np.exp(-t / 0.07)
    difference = loss.regions(altered) - loss.target
    assert difference[2, 0] > 1  # Added early upper-band energy is visible in dB.
    assert abs(difference[0, 0]) < 0.001  # It does not fabricate low-body energy.
    assert abs(difference[2, -1]) < 0.001  # Or a longer high-frequency tail.
