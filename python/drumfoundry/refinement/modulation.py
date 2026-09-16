"""Level-independent band-envelope diagnostics, separate from decay fitting.

This measures fluctuations around a smooth decay, not perceptual similarity.
Always inspect spectral density and ridge structure alongside this number: a
sparse beating tone can match the fluctuation statistic but sound very wrong.
"""

import numpy as np
from scipy.ndimage import gaussian_filter1d
from scipy.signal import butter, sosfilt


def band_fluctuation_db(
    audio, rate, band, region, *, window_seconds=0.005, trend_seconds=0.05
):
    """RMS dB fluctuations after causal filtering and Gaussian detrending.

    Use the same band, time region and window settings for both signals. The
    input is neither normalized nor modified. Nearly silent regions should be
    excluded by the caller; this statistic does not detect their audibility.
    """
    audio = np.asarray(audio, dtype=float)
    low, high = band
    start, end = region
    values = (rate, low, high, start, end, window_seconds, trend_seconds)
    if (
        not np.isfinite(values).all()
        or rate <= 0
        or not 0 < low < high < rate / 2
        or not 0 <= start < end
        or not 0 < window_seconds <= trend_seconds
        or audio.ndim != 1
        or not np.isfinite(audio).all()
        or end > audio.size / rate
    ):
        raise ValueError("Invalid band-envelope diagnostic input")
    hop = max(1, round(window_seconds * rate))
    count = audio.size // hop
    centres = (np.arange(count) + 0.5) * hop / rate
    keep = (centres >= start) & (centres < end)
    if not np.any(keep):
        raise ValueError("Analysis region contains no complete power windows")
    filtered = sosfilt(butter(4, band, fs=rate, btype="bandpass", output="sos"), audio)
    power = np.mean(filtered[: count * hop].reshape(count, hop) ** 2, axis=1)
    envelope = 10 * np.log10(np.maximum(power, 1e-20))
    trend = gaussian_filter1d(envelope, trend_seconds * rate / hop)
    return float(np.sqrt(np.mean((envelope[keep] - trend[keep]) ** 2)))
