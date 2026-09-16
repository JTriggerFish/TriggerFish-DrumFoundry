"""Active-region spectral matching with a separate line-versus-noise distance.

These are engineering diagnostics, not a claim of perceptual equivalence.
One constant analysis-only gain offset removes unknown library velocity gain.
"""

import numpy as np
from scipy.ndimage import gaussian_filter1d
from scipy.signal import welch
from .layer_loss import LayerLoss, REGIONS


def texture_bands(rate):
    """Include audible upper ridges, clipping safely below Nyquist."""
    upper = min(20000, 0.48 * rate)
    return tuple(
        (lo, min(hi, upper))
        for lo, hi in (
            (250, 750),
            (750, 1500),
            (1500, 3000),
            (3000, 6000),
            (6000, 14000),
            (14000, 17000),
            (17000, 20000),
        )
        if lo < upper
    )


def line_texture(audio, rate):
    """Position-independent ridge contrast in octave bands, after local whitening."""
    rows = []
    for start, end in ((0.08, 0.55), (0.55, 1.3)):
        segment = audio[round(start * rate) : round(end * rate)]
        f, power = welch(segment, rate, nperseg=len(segment), noverlap=0)
        floor = max(float(power.max()) * 1e-7, 1e-25)
        colour = gaussian_filter1d(power, 50 / (f[1] - f[0]))
        whitened = np.maximum(power, floor) / np.maximum(colour, floor)
        for lo, hi in texture_bands(rate):
            values = whitened[(f >= lo) & (f < hi)]
            rows.append(10 * (np.mean(np.log10(values)) - np.log10(np.mean(values))))
    return np.asarray(rows)


class TonalLayerLoss(LayerLoss):
    """Do not dilute an inaccurate attack by averaging in matching silence."""

    def __init__(self, reference, rate):
        super().__init__(reference, rate, "timbre")
        self.target_texture = line_texture(reference, rate)
        self.counts = [
            max(1, int(np.count_nonzero(t > 10 * np.log10(f) + 6)))
            for _, t, f in self.blocks
        ]
        self.active_blocks = max(1, sum(c > 1 for c in self.counts))
        self.target_shape = self.spectral_shape(reference)
        edges = np.geomspace(70, min(20000, 0.48 * rate), 97)
        # Broad resonances carrying real energy must not be outvoted by many
        # quiet bins. Keep uniform weight too, so excess elsewhere still counts.
        salient = np.sqrt(self.target_shape * np.diff(edges)[None, :])
        salient /= np.maximum(salient.mean(axis=1, keepdims=True), 1e-25)
        self.shape_weights = 0.3 + 0.7 * salient
        self.specification = dict(
            self.specification,
            version="tonal-layer-v3",
            active_normalization="reference bins above floor+6dB; silent excess still penalized",
            components={"envelope": 1, "spectral_shape": 1, "ridge_contrast": 2},
            texture="locally whitened line contrast; no upper peak identities",
            shape_weights="30% uniform + 70% reference sqrt band energy",
            shape_regions_seconds=[[0.03, 0.15], [0.15, 0.5], [0.5, 1.2]],
            shape_bands=96,
            shape_hz=[70, min(20000, 0.48 * rate)],
            texture_regions_seconds=[[0.08, 0.55], [0.55, 1.3]],
            texture_bands_hz=texture_bands(rate),
            texture_whitening_hz=50,
        )

    def spectral_shape(self, audio):
        """Log-frequency pooled power resolves broad low resonances, not high peaks."""
        shapes = []
        for lo, hi in ((0.03, 0.15), (0.15, 0.5), (0.5, 1.2)):
            segment = audio[round(lo * self.rate) : round(hi * self.rate)]
            f, power = welch(
                segment, self.rate, nperseg=min(8192, len(segment)), nfft=16384
            )
            edges = np.geomspace(70, min(20000, 0.48 * self.rate), 97)
            shapes.append(
                np.array(
                    [
                        np.mean(power[(f >= a) & (f < b)])
                        for a, b in zip(edges[:-1], edges[1:])
                    ]
                )
            )
        return np.asarray(shapes)

    def components(self, audio):
        """Return independently inspectable errors; none is a sound-quality verdict."""
        compared, _ = self.comparison_audio(audio)
        values = self.represent(compared)
        squared = []
        for i, (power, _) in enumerate(values):
            for j, (mask, target, floor) in enumerate(
                self.blocks[i * len(REGIONS) : (i + 1) * len(REGIONS)]
            ):
                delta = 10 * np.log10(np.maximum(power[:, mask], floor)) - target
                count = max(self.counts[i * len(REGIONS) + j], delta.size * 0.02)
                squared.append(float(np.sum(delta**2) / count))
        envelope = np.sqrt(sum(squared) / self.active_blocks)
        shape = self.spectral_shape(compared)
        floor = np.maximum(self.target_shape.max(axis=1, keepdims=True) * 1e-5, 1e-25)
        delta = 10 * np.log10(
            np.maximum(shape, floor) / np.maximum(self.target_shape, floor)
        )
        spectral = float(np.sqrt(np.mean(delta**2 * self.shape_weights)))
        texture = float(
            np.sqrt(
                np.mean((line_texture(compared, self.rate) - self.target_texture) ** 2)
            )
        )
        return dict(
            envelope=float(envelope), spectral_shape=spectral, ridge_contrast=texture
        )

    def diagnostics(self, audio):
        parts = self.components(audio)
        return dict(
            error_db=float(
                np.sqrt(
                    (
                        parts["envelope"] ** 2
                        + parts["spectral_shape"] ** 2
                        + 4 * parts["ridge_contrast"] ** 2
                    )
                    / 6
                )
            ),
            comparison_gain_db=self.comparison_audio(audio)[1],
            **parts,
        )

    def residual(self, audio):
        """Expose the same objective to callers using the residual interface."""
        parts = self.components(audio)
        return np.array(
            [parts["envelope"], parts["spectral_shape"], 2 * parts["ridge_contrast"]]
        ) / np.sqrt(6)
