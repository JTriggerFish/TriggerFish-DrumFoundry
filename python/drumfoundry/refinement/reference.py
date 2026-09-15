"""Explicit local reference preparation; hash, gain and alignment are never guessed."""

from dataclasses import dataclass
from hashlib import sha256
from pathlib import Path
import math

import numpy as np
from triggerfish_percussion.audio_io import read_wav, resample_audio


@dataclass
class Reference:
    samples: np.ndarray
    rate: int
    provenance: dict

    def window(self, seconds):
        """Crop/pad the already aligned reference; retain absolute amplitude."""
        if not math.isfinite(seconds) or not 0 < seconds <= 120:
            raise ValueError("Reference duration must be in (0, 120]")
        count = math.floor(seconds * self.rate + 0.5)
        return np.pad(self.samples[:count], (0, max(0, count - len(self.samples))))


def load_reference(fit, path, rate, *, gain_db=None, onset=None, channel=None):
    """Use saved presentation or explicit overrides, with a recorded raw-file hash."""
    if isinstance(rate, bool) or int(rate) != rate or not 16000 <= rate <= 192000:
        raise ValueError("Reference render rate must be an integer in 16–192 kHz")
    path = Path(path)
    attachment = fit.get("reference") or {}
    digest = sha256(path.read_bytes()).hexdigest()
    if attachment.get("sha256") and attachment["sha256"] != digest:
        raise ValueError("Reference file hash differs from saved fit")
    gain_db = attachment.get("referenceGainDb", 0) if gain_db is None else gain_db
    onset = (
        attachment.get(
            "offsetSeconds", (attachment.get("cell") or {}).get("onset_seconds", 0)
        )
        if onset is None
        else onset
    )
    channel = attachment.get("channel", 0) if channel is None else channel
    if (
        type(channel) is not int
        or channel not in (0, 1, 2)
        or not math.isfinite(gain_db)
        or not -60 <= gain_db <= 48
    ):
        raise ValueError("Invalid reference channel or gain")
    if not math.isfinite(onset) or onset < 0:
        raise ValueError("Reference onset must be finite and nonnegative")
    audio = read_wav(path, ("mean", "left", "right")[channel])
    source_rate = audio.sample_rate
    audio = resample_audio(audio, rate)
    start = math.floor(onset * rate + 0.5)
    if start >= len(audio.samples):
        raise ValueError("Reference onset is outside the recording")
    samples = audio.samples[start:] * 10 ** (gain_db / 20)
    if not np.isfinite(samples).all() or not np.any(samples):
        raise ValueError("Reference must contain finite, nonzero audio")
    return Reference(
        samples,
        rate,
        dict(
            filename=path.name,
            sha256=digest,
            source_rate=source_rate,
            render_rate=rate,
            gain_db=gain_db,
            onset_seconds=onset,
            channel=channel,
            resampling="none" if source_rate == rate else "scipy.resample_poly/line",
            normalization=False,
            aligned_pcm_sha256=sha256(samples.astype("<f8").tobytes()).hexdigest(),
        ),
    )


def reference_attachment(fit, reference, path, library_root=None):
    """Retain a resolvable UI attachment, never guess an absolute sample path."""
    attachment = dict(fit.get("reference") or {})
    if library_root is not None:
        attachment["libraryPath"] = (
            Path(path).resolve().relative_to(Path(library_root).resolve()).as_posix()
        )
    elif attachment.get("sha256") != reference.provenance["sha256"]:
        raise ValueError(
            "Provide --library-root to establish an unverified reference attachment"
        )
    if not attachment.get("libraryPath"):
        raise ValueError(
            "Provide --library-root when the fit has no reference libraryPath"
        )
    p = reference.provenance
    attachment.update(
        sha256=p["sha256"],
        referenceGainDb=p["gain_db"],
        offsetSeconds=p["onset_seconds"],
        channel=p["channel"],
    )
    return attachment
