"""Causal onset, bloom and decay comparison, adapted from the gong experiment.

All three phases remain separately inspectable. A constant analysis-only gain
offset accommodates unknown sample-library velocity gain; no time-varying gain
or band normalization is applied to the audio.
"""

import numpy as np
from scipy.signal import butter, sosfilt, welch
from .tonal_layer_loss import line_texture, texture_bands

BANDS = (
    (80, 250),
    (250, 800),
    (800, 1500),
    (1500, 3000),
    (3000, 5000),
    (5000, 9000),
    (9000, 14000),
    (14000, 17000),
    (17000, 20000),
)
REGIONS = ((0, 0.1), (0.1, 0.35), (0.35, 1.5), (1.5, 3))
WEIGHTS = dict(
    onset=1, bloom=1, decay=1, tail=1, rise=1, spectrum=0.3, body_ridge=1, texture=0.1
)


def residual_parts(parts):
    """One authoritative vector/diagnostic weighting for every solver."""
    return np.concatenate(
        [x * np.sqrt(WEIGHTS[k] / x.size) for k, x in parts.items()]
    ) / np.sqrt(sum(WEIGHTS.values()))


class TemporalMetalLoss:
    """Fit temporal evolution first; spectral texture cannot substitute for it."""

    level_policy = "timbre"

    def __init__(self, reference, rate):
        self.rate = rate
        self.reference = np.asarray(reference, dtype=float)
        if rate <= 28000 or self.reference.shape != (round(3 * rate),):
            raise ValueError("Expected three seconds of mono audio above 28 kHz")
        if not np.isfinite(self.reference).all() or not np.any(self.reference):
            raise ValueError("Expected finite nonzero reference")
        upper = min(20000, 0.48 * rate)
        self.bands = tuple((lo, min(hi, upper)) for lo, hi in BANDS if lo < upper)
        self.spectral_edges = np.geomspace(70, upper, 81)
        self.filters = [
            butter(4, band, fs=rate, btype="bandpass", output="sos")
            for band in self.bands
        ]
        self.hop = round(0.01 * rate)
        self.times = (np.arange(len(reference) // self.hop) + 0.5) * self.hop / rate
        self.target_power = self.power(reference)
        self.floor = max(self.target_power.max() * 1e-5, 1e-24)
        self.target_db = self.db(self.target_power)
        self.target_spectrum = self.spectrum(reference)
        self.target_texture = line_texture(reference, rate)
        self.specification = dict(
            version="temporal-metal-v4",
            bands_hz=self.bands,
            spectrum_hz=[70, upper],
            texture_bands_hz=texture_bands(rate),
            causal_filter="fourth-order Butterworth",
            rms_window_seconds=0.01,
            regions_seconds=REGIONS,
            floor_db=-50,
            spectrum_regions_seconds=((0.02, 0.1), (0.1, 0.3), (0.3, 1)),
            normalization="single analysis-only whole-render RMS offset",
            components=WEIGHTS,
            body_ridge="early 70-1500 Hz magnitude spectral convergence, scaled by 20/ln(10)",
            acceptance="Inspect every phase and layer; aggregate is search ranking only",
        )

    def comparison_audio(self, audio):
        audio = np.asarray(audio, dtype=float)
        if audio.shape != self.reference.shape or not np.isfinite(audio).all():
            raise ValueError("Expected finite audio matching reference shape")
        energy = float(audio @ audio)
        gain = np.sqrt(float(self.reference @ self.reference) / energy) if energy else 1
        return audio * gain, float(20 * np.log10(gain))

    def power(self, audio):
        count = len(audio) // self.hop
        return np.asarray(
            [
                np.mean(
                    sosfilt(f, audio)[: count * self.hop].reshape(count, self.hop) ** 2,
                    axis=1,
                )
                for f in self.filters
            ]
        )

    def db(self, power):
        return 10 * np.log10(np.maximum(power, self.floor))

    def spectrum(self, audio):
        """Resolve the initial low body without fitting individual upper peaks."""
        rows = []
        edges = self.spectral_edges
        for a, b in ((0.02, 0.1), (0.1, 0.3), (0.3, 1)):
            segment = audio[round(a * self.rate) : round(b * self.rate)]
            f, p = welch(
                segment, self.rate, nperseg=min(8192, len(segment)), nfft=16384
            )
            rows.append(
                [
                    np.mean(p[(f >= lo) & (f < hi)])
                    for lo, hi in zip(edges[:-1], edges[1:])
                ]
            )
        return np.asarray(rows)

    def parts(self, audio):
        compared, _ = self.comparison_audio(audio)
        actual = self.db(self.power(compared))
        delta = actual - self.target_db
        result = {}
        for name, (a, b) in zip(("onset", "bloom", "decay", "tail"), REGIONS):
            result[name] = delta[:, (self.times >= a) & (self.times < b)].ravel()
        # Explicit within-band rise: matching an averaged spectrum cannot win
        # by replacing a later bloom with equally loud premature high energy.
        early = delta[:, self.times < 0.03].mean(axis=1, keepdims=True)
        result["rise"] = (
            delta[:, (self.times >= 0.03) & (self.times < 0.3)] - early
        ).ravel()
        floor = np.maximum(
            self.target_spectrum.max(axis=1, keepdims=True) * 1e-5, 1e-24
        )
        spectrum = self.spectrum(compared)
        result["spectrum"] = (
            10
            * np.log10(
                np.maximum(spectrum, floor) / np.maximum(self.target_spectrum, floor)
            )
        ).ravel()
        centres = np.sqrt(self.spectral_edges[:-1] * self.spectral_edges[1:])
        low = centres < 1500
        target = np.sqrt(self.target_spectrum[:2, low])
        # Log-only spectral errors overvalue quiet valleys. This complementary
        # magnitude convergence preserves a salient initial tone, without
        # assigning free fitted frequencies to any upper ridges.
        result["body_ridge"] = (
            20
            / np.log(10)
            * (np.sqrt(spectrum[:2, low]) - target)
            / np.maximum(
                np.sqrt(np.mean(target * target, axis=1, keepdims=True)), 1e-20
            )
        ).ravel()
        result["texture"] = line_texture(compared, self.rate) - self.target_texture
        return result

    def residual(self, audio):
        parts = self.parts(audio)
        return residual_parts(parts)

    def diagnostics(self, audio):
        parts = {
            k: float(np.sqrt(np.mean(x * x))) for k, x in self.parts(audio).items()
        }
        return dict(
            error_db=float(
                np.sqrt(
                    sum(WEIGHTS[k] * v * v for k, v in parts.items())
                    / sum(WEIGHTS.values())
                )
            ),
            comparison_gain_db=self.comparison_audio(audio)[1],
            **parts,
        )
