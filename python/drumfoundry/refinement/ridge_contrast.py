"""Descriptive metallic ridge contrast, independent of the search objective."""

import numpy as np
from scipy.signal import welch
from scipy.ndimage import gaussian_filter1d


def ridge_contrast(audio, rate):
    """Descriptive line-versus-wash statistic after removing broad spectral colour.

    One-second Hann spectra, locally whitened by a 50-Hz Gaussian power average.
    Negative dB means more concentrated ridges; not an acceptance loss.
    """
    rows = []
    for start in (0.1, 0.6, 1.2):
        segment = audio[round(start * rate) : round((start + 1) * rate)]
        f, power = welch(segment, rate, nperseg=len(segment), noverlap=0)
        floor = max(float(power.max()) * 1e-6, 1e-25)
        shape = gaussian_filter1d(power, 50 / (f[1] - f[0]))
        whitened = np.maximum(power, floor) / np.maximum(shape, floor)
        row = []
        for low, high in ((1500, 3000), (3000, 6000), (6000, 12000)):
            values = whitened[(f >= low) & (f < high) & (shape > floor * 10)]
            row.append(
                float(10 * np.log10(np.exp(np.mean(np.log(values))) / np.mean(values)))
                if len(values)
                else None
            )
        rows.append(row)
    return dict(
        regions_seconds=[[0.1, 1.1], [0.6, 1.6], [1.2, 2.2]],
        bands_hz=[[1500, 3000], [3000, 6000], [6000, 12000]],
        flatness_db=rows,
    )
