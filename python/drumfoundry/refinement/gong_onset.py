"""Causal-band onset/bloom diagnostic from the final gong onset experiment."""

import numpy as np
from scipy.signal import butter, sosfilt

BANDS = [(80, 800), (2500, 5000), (5000, 9000), (9000, 14000)]


def measure(audio, rate):
    """10 ms disjoint RMS windows after causal fourth-order band filtering."""
    hop = round(0.01 * rate)
    frames = len(audio) // hop
    t = (np.arange(frames) + 0.5) * hop / rate
    curves, rows = [], []
    for lo, hi in BANDS:
        signal = sosfilt(
            butter(4, [lo, hi], btype="bandpass", fs=rate, output="sos"), audio
        )
        power = np.mean(signal[: frames * hop].reshape(frames, hop) ** 2, axis=1)
        db = 10 * np.log10(np.maximum(power, 1e-30))
        peak = np.argmax(power[(t >= 0.1) & (t < 2)]) + np.flatnonzero(t >= 0.1)[0]

        def window(start, end):
            return float(
                10 * np.log10(max(1e-30, np.mean(power[(t >= start) & (t < end)])))
            )

        rows.append(
            dict(
                band=[lo, hi],
                first_20ms_db=window(0, 0.02),
                early_20_100ms_db=window(0.02, 0.1),
                bloom_300_1000ms_db=window(0.3, 1),
                peak_db=float(db[peak]),
                peak_seconds=float(t[peak]),
            )
        )
        curves.append(db)
    return t, curves, rows


class GongOnsetLoss:
    """Keep low body and quiet early highs visible; do not fit the painted mid scoop."""

    def __init__(self, reference, rate):
        if rate <= 28000 or len(reference) < 3 * rate:
            raise ValueError("Gong onset analysis needs three seconds above 28 kHz")
        self.rate, self.frames = rate, len(reference)
        self.target = self.regions(reference)
        self.specification = dict(
            version="gong-onset-regions-v1",
            bands_hz=BANDS,
            regions_seconds=[(0.02, 0.1), (0.1, 0.3), (0.3, 1), (1, 2), (2, 3)],
            selected_bands=[0, 2, 3],
            normalization=False,
            filtering="causal fourth-order Butterworth",
        )

    def regions(self, audio):
        if np.shape(audio) != (self.frames,) or not np.isfinite(audio).all():
            raise ValueError("Expected finite mono audio matching the reference")
        t, curves, _ = measure(audio, self.rate)
        power = 10 ** (np.asarray(curves) / 10)
        return np.asarray(
            [
                10
                * np.log10(np.maximum(1e-30, power[:, (t >= a) & (t < b)].mean(axis=1)))
                for a, b in ((0.02, 0.1), (0.1, 0.3), (0.3, 1), (1, 2), (2, 3))
            ]
        ).T

    def residual(self, audio):
        return (self.regions(audio) - self.target)[[0, 2, 3]].ravel() / np.sqrt(15)

    def diagnostics(self, audio):
        return dict(
            regions_db=self.regions(audio).tolist(),
            difference_db=(self.regions(audio) - self.target).tolist(),
        )
