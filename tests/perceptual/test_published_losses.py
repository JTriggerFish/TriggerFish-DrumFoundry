"""Opt-in dependency/API smoke tests, not perceptual fit acceptance tests."""

import numpy as np
import pytest

from triggerfish_percussion.perceptual_fit_losses import AuralossMel, JtfsLoss


@pytest.mark.parametrize("factory", [AuralossMel, JtfsLoss])
def test_identical_signal_beats_a_changed_signal(factory):
    import torch

    torch.set_num_threads(2)
    rate = 16000
    t = np.arange(rate, dtype=np.float32) / rate
    reference = (0.2 * np.sin(2 * np.pi * 211 * t) * np.exp(-4 * t)).astype(np.float32)
    loss = factory(reference, float(rate))  # The native renderer exposes a float.
    same = loss.score(reference)
    changed = loss.score(reference * 0.5)
    assert np.isfinite([same, changed]).all()
    assert same < 1e-8
    assert changed > same + 1e-6
