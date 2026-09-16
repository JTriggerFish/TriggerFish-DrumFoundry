"""A texture diagnostic must distinguish movement from gain and smooth decay."""

import numpy as np
import pytest
from drumfoundry.refinement.modulation import band_fluctuation_db


def test_band_fluctuations_ignore_gain_and_smooth_decay():
    rate = 48000
    t = np.arange(rate * 2) / rate
    tone = np.sin(2 * np.pi * 14000 * t)
    smooth = tone * np.exp(-2 * t)
    moving = smooth * (1 + 0.6 * np.sin(2 * np.pi * 15 * t))

    def measure(x):
        return band_fluctuation_db(x, rate, (12000, 20000), (0.2, 1.5))

    assert measure(smooth) < 0.01
    assert measure(moving) > 3
    assert measure(moving * 0.1) == pytest.approx(measure(moving), abs=1e-8)


@pytest.mark.parametrize(
    "band,region",
    [((0, 20000), (0.2, 1)), ((12000, 24000), (0.2, 1)), ((12000, 20000), (1, 3))],
)
def test_invalid_band_or_region_is_explicit(band, region):
    with pytest.raises(ValueError):
        band_fluctuation_db(np.zeros(48000 * 2), 48000, band, region)
