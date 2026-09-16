"""Reference-fixed, region-balanced spectral envelopes for multi-layer fitting.

ERB pooling deliberately compares upper spectral density, not unidentified
individual cymbal peaks. This is an engineering loss, not listening approval.
"""

import numpy as np
from scipy.signal import stft

REGIONS = ((0, 0.03), (0.03, 0.1), (0.1, 0.3), (0.3, 0.7), (0.7, 1.5), (1.5, 3))


class LayerLoss:
    """Equal time-region weight, fixed reference floor, no playback normalization."""

    def __init__(self, reference, rate, level_policy="absolute"):
        reference = np.asarray(reference)
        if (
            not np.isfinite(rate)
            or not 16000 <= rate <= 192000
            or reference.ndim != 1
            or len(reference) < 3 * rate
            or not np.isfinite(reference).all()
            or not np.any(reference)
        ):
            raise ValueError(
                "Expected at least three seconds of finite nonzero mono audio"
            )
        self.rate = rate
        self.length = len(reference)
        if level_policy not in ("absolute", "timbre"):
            raise ValueError("Unknown level comparison policy")
        self.level_policy = level_policy
        self.reference_energy = float(np.sum(reference**2))
        self.transforms = []
        for size, bands in ((512, 24), (2048, 48)):
            freq = np.fft.rfftfreq(size, 1 / rate)
            erb = 21.4 * np.log10(1 + 0.00437 * freq)
            edges = np.linspace(21.4 * np.log10(1 + 0.00437 * 80), erb[-1], bands + 2)
            matrix = np.maximum(
                0,
                np.minimum(
                    (erb[None] - edges[:-2, None]) / np.diff(edges)[:-1, None],
                    (edges[2:, None] - erb[None]) / np.diff(edges)[1:, None],
                ),
            )
            self.transforms.append((size, matrix))
        self.targets = self.represent(reference)
        self.blocks = []
        for power, times in self.targets:
            floor = max(float(power.max()) * 1e-5, 1e-24)
            target = 10 * np.log10(np.maximum(power, floor))
            # All frequencies remain scored, including excess candidate energy.
            for lo, hi in REGIONS:
                mask = (times >= lo) & (times < hi)
                if not mask.any():
                    raise ValueError("Layer objective requires three seconds of audio")
                self.blocks.append((mask, target[:, mask], floor))
        self.specification = dict(
            version="layer-ERB-envelope-v1",
            fft=[512, 2048],
            erb_bands=[24, 48],
            hop_fraction=0.25,
            regions=REGIONS,
            floor_db=-50,
            region_weights="equal",
            normalization=(
                "analysis-only RMS offset" if level_policy == "timbre" else False
            ),
            level_policy=level_policy,
            comparison_gain=(
                "one RMS offset per layer" if level_policy == "timbre" else "none"
            ),
            playback_gain_matching=False,
        )

    def comparison_audio(self, audio):
        """Remove pure level only in analysis; no envelope compression or DSP edit."""
        audio = np.asarray(audio, dtype=float)
        if audio.shape != (self.length,) or not np.isfinite(audio).all():
            raise ValueError("Expected equal-length finite mono audio")
        energy = float(np.sum(audio**2))
        if self.level_policy == "timbre" and energy > 0:
            gain = np.sqrt(self.reference_energy / energy)
            return audio * gain, float(20 * np.log10(gain))
        return audio, 0.0

    def represent(self, audio):
        """Pool linear STFT power before taking logarithms."""
        audio = np.asarray(audio)
        if audio.ndim != 1 or len(audio) != self.length or not np.isfinite(audio).all():
            raise ValueError("Expected equal-length finite mono audio")
        result = []
        for size, matrix in self.transforms:
            _, times, spectrum = stft(
                audio,
                self.rate,
                nperseg=size,
                noverlap=3 * size // 4,
                boundary="zeros",
                padded=True,
            )
            result.append((matrix @ np.abs(spectrum) ** 2, times))
        return result

    def residual(self, audio):
        """RMS norm is dB error, independent of bin and frame counts."""
        values = self.represent(self.comparison_audio(audio)[0])
        result = []
        for i, (power, _) in enumerate(values):
            for mask, target, floor in self.blocks[
                i * len(REGIONS) : (i + 1) * len(REGIONS)
            ]:
                error = 10 * np.log10(np.maximum(power[:, mask], floor)) - target
                result.append(error.ravel() / np.sqrt(error.size * len(self.blocks)))
        return np.concatenate(result)

    def diagnostics(self, audio):
        residual = self.residual(audio)
        return dict(
            error_db=float(np.linalg.norm(residual)),
            comparison_gain_db=self.comparison_audio(audio)[1],
        )
