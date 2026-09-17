"""Read-only migration comparisons against externally stored legacy PCM.

No legacy runtime is imported. Compact temporal/spectral signatures can also
be checked in CI without distributing full oracle renders or private samples.
"""

import argparse
from copy import deepcopy
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
    instrument = deepcopy(document["instrument"])
    # This is only a historical hash adapter, not a second parameter/DSP map.
    # Native loading moves the unchanged metallic contact values to a module;
    # the old PCM oracle hashed them under body. Preserve that oracle identity.
    if instrument["recipe"] == "metal.cymbal.v1":
        nodes = instrument["nodes"]
        module = next((n for n in nodes if n["id"] == "rim-contact"), None)
        if module is not None:
            body = next(n for n in nodes if n["id"] == "body")
            body["parameters"].update(module["parameters"])
            nodes.remove(module)
            instrument.pop("attachments", None)
    # The historical endpoint was fixed at 15 kHz. Its explicit neutral value
    # is equivalent to absence; other values must still change the identity.
    # This keeps the original audio oracles/hashes, rather than regenerating them.
    for node in instrument["nodes"]:
        params = node.get("parameters", {})
        if params.get("body_decay_frequency_7") == 15000:
            del params["body_decay_frequency_7"]
        # The final bell previously had fixed Q=0.7. Exposing that exact value
        # changes serialization, not sound; retain the original audio oracle.
        if params.get("output_colour_q") == 0.7:
            del params["output_colour_q"]
        # Optional loss and direct strike accent are exact bypasses at zero.
        # Keep the original PCM/signature oracle rather than regenerating it.
        if params.get("body_decay_friction") == 0:
            del params["body_decay_friction"]
        if params.get("hat_contact_enabled", 0) == 0:
            for key in (
                "hat_contact_enabled",
                "hat_openness",
                "hat_clearance",
                "hat_contact_loss",
                "hat_pedal_strength",
                "hat_rattle_motion",
                "hat_settling",
            ):
                params.pop(key, None)
        if params.get("contact_noise_level", 0) == 0:
            for key in (
                "contact_noise_level",
                "contact_noise_decay",
                "contact_noise_colour",
            ):
                params.pop(key, None)
    sound = {
        "instrument": instrument,
        "event": document["controls"]["event"],
    }
    encoded = json.dumps(sound, sort_keys=True, separators=(",", ":"), allow_nan=False)
    return hashlib.sha256(encoded.encode("utf8")).hexdigest()


def render_case(presets, case):
    """Render the manifest's original patch, verifying identity before use.

    Archived cases declare a preset_file relative to the supplied preset folder.
    Do not guess/fall back to an archive on hash mismatch: that could conceal a
    real, unintended preset edit.
    """
    preset = Path(presets) / case.get("preset_file", f"{case['preset']}.fit.json")
    content = preset.read_bytes().replace(b"\r\n", b"\n")
    identity = (
        sound_identity(json.loads(content))
        if "sound_sha256" in case
        else hashlib.sha256(content).hexdigest()
    )
    expected = (
        case["sound_sha256"] if "sound_sha256" in case else case["preset_sha256_lf"]
    )
    if identity != expected:
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
