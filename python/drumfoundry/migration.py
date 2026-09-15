"""Read-only migration comparisons against externally stored legacy PCM.

No legacy runtime is imported. Compact temporal/spectral signatures can also
be checked in CI without distributing full oracle renders or private samples.
"""

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np

from .renderer import Renderer

TIME_EDGES = (0, 0.01, 0.03, 0.1, 0.3, 0.6, 1, 2, 4, 6)
BAND_EDGES = (0, 40, 100, 250, 500, 1000, 2000, 4000, 8000, 16000, 24000)


def signature(audio, rate):
    """Absolute RMS by time region and frequency band; no level normalization."""
    audio = np.asarray(audio, dtype=np.float64)
    if len(audio) != 6 * rate or not np.isfinite(audio).all():
        raise ValueError("Expected six seconds of finite mono PCM")
    temporal = [
        float(np.sqrt(np.mean(audio[round(a * rate) : round(b * rate)] ** 2)))
        for a, b in zip(TIME_EDGES[:-1], TIME_EDGES[1:])
    ]
    power = np.abs(np.fft.rfft(audio)) ** 2 / len(audio) ** 2
    power[1:-1] *= 2  # All supported migration lengths are even.
    frequencies = np.fft.rfftfreq(len(audio), 1 / rate)
    spectral = [
        float(np.sqrt(np.sum(power[(frequencies >= a) & (frequencies < b)])))
        for a, b in zip(BAND_EDGES[:-1], BAND_EDGES[1:])
    ]
    return {"temporal_rms": temporal, "spectral_rms": spectral}


def sound_identity(document):
    """Hash the DSP patch and strike defaults, not reference/view metadata."""
    sound = {
        "instrument": document["instrument"],
        "event": document["controls"]["event"],
    }
    encoded = json.dumps(sound, sort_keys=True, separators=(",", ":"), allow_nan=False)
    return hashlib.sha256(encoded.encode("utf8")).hexdigest()


def render_case(presets, case):
    """Enforce the original patch identity before comparing a saved baseline."""
    preset = Path(presets) / f"{case['preset']}.fit.json"
    content = preset.read_bytes().replace(b"\r\n", b"\n")
    identity = (
        sound_identity(json.loads(content))
        if "sound_sha256" in case
        else hashlib.sha256(content).hexdigest()
    )
    if identity != case.get("sound_sha256", case["preset_sha256_lf"]):
        raise ValueError(f"Migration preset changed: {preset.name}")
    events = [{"time": t} for t in (0, 0.5, 1, 1.5, 2)] if case["repeated"] else None
    with Renderer(preset, case["rate"]) as renderer:
        return renderer.render(6, events)


def compare_oracles(baseline, presets, oracles):
    """Compare every sample where the optional original float32 files exist."""
    rows = []
    for case in baseline["cases"]:
        name = f"{case['preset']}-{case['rate']}-{str(case['repeated']).lower()}.f32"
        content = (Path(oracles) / name).read_bytes()
        if hashlib.sha256(content).hexdigest() != case["oracle_sha256"]:
            raise ValueError(f"Oracle identity mismatch: {name}")
        reference = np.frombuffer(content, dtype="<f4").astype(np.float64)
        actual = render_case(presets, case)
        error = actual - reference
        rms = float(np.linalg.norm(error) / np.linalg.norm(reference))
        if not np.isfinite(rms) or rms > 0.002:
            raise ValueError(f"Migration parity failed for {name}: relative RMS {rms}")
        rows.append(
            {
                "case": name,
                "relative_rms": rms,
                "max_error": float(np.max(np.abs(error))),
            }
        )
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("presets", type=Path)
    parser.add_argument("oracles", type=Path)
    args = parser.parse_args()
    baseline = json.loads(args.baseline.read_text(encoding="utf8"))
    print(json.dumps(compare_oracles(baseline, args.presets, args.oracles), indent=2))


if __name__ == "__main__":
    main()
