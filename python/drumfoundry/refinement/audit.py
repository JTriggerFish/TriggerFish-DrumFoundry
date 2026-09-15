"""Independent native audit of frozen source/candidate fits, not today's preset."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import numpy as np

from drumfoundry._native import library_path
from triggerfish_percussion.audio_io import AudioBuffer, write_wav
from .contracts import audit_seeds, prepare_output
from .reference import load_reference
from .saved import SavedFitRenderer
from .objective import objective_for
from .artifacts import write_json
from .configuration import duration


def audit(directory, reference_path, output):
    """Record separate losses, exact reload, velocity/restrike checks and raw peaks."""
    archive = json.loads((directory / "report.json").read_text(encoding="utf8"))
    fits = {
        name: json.loads((directory / file).read_text(encoding="utf8"))
        for name, file in (
            ("before", "source.fit.json"),
            ("candidate", "candidate.fit.json"),
        )
    }
    identities = {
        name: sha256((directory / file).read_bytes()).hexdigest()
        for name, file in (
            ("before", "source.fit.json"),
            ("candidate", "candidate.fit.json"),
        )
    }
    if identities != archive["fits_sha256"]:
        raise ValueError("Frozen fit changed since fitting; start a new run")
    primary = [fit["controls"]["event"]["seed"] for fit in fits.values()]
    seeds = audit_seeds(*primary, *archive["training_seeds"])
    conditioning = archive["reference"]
    rate = conditioning["render_rate"]
    seconds = duration(archive["profile"])
    if archive.get("duration_seconds", seconds) != seconds:
        raise ValueError("Archived duration differs from the objective windows")
    ref = load_reference(
        fits["candidate"],
        reference_path,
        rate,
        gain_db=conditioning["gain_db"],
        onset=conditioning["onset_seconds"],
        channel=conditioning["channel"],
    )
    if any(
        ref.provenance[k] != conditioning[k]
        for k in ("sha256", "source_rate", "aligned_pcm_sha256", "resampling")
    ):
        raise ValueError("Audit reference differs from the archived fitting reference")
    target = ref.window(seconds)
    with SavedFitRenderer(fits["before"], rate) as original:
        baseline = original.render(original.initial, seconds)
    loss, components = objective_for(archive["profile"], target, baseline, rate)
    if json.loads(json.dumps(loss.specification)) != archive["objective"]:
        raise ValueError("Audit objective changed; do not compare incompatible scores")
    prepare_output(output)
    result = dict(
        profile=archive["profile"],
        reference=ref.provenance,
        objective=loss.specification,
        training_seeds=archive["training_seeds"],
        audit_seeds=seeds,
        renderer_sha256=sha256(library_path().read_bytes()).hexdigest(),
        fitting_renderer_sha256=archive["renderer_sha256"],
        fits_sha256=identities,
        acceptance="diagnostic only; listening approval required",
    )
    write_wav(output / "reference.wav", AudioBuffer(target, rate))
    if seconds >= 6:
        from triggerfish_percussion.modulation_signature import modulation_signature
        from .ridge_contrast import ridge_contrast

        result["reference_texture"] = dict(
            motion=modulation_signature(target, rate),
            ridges=ridge_contrast(target, rate),
        )
    for name, fit in fits.items():
        with SavedFitRenderer(fit, rate) as saved:
            audio = saved.render(saved.initial, seconds)
            with SavedFitRenderer(saved.snapshot(saved.initial), rate) as reloaded:
                if not np.array_equal(
                    audio, reloaded.render(reloaded.initial, seconds)
                ):
                    raise ValueError("Saved fit reload changed the rendered sound")
            metrics = [
                dict(
                    seed=seed, **components(saved.render(saved.initial, seconds, seed))
                )
                for seed in seeds
            ]
            result[name] = dict(metrics=metrics, exact_reload=True)
            if seconds >= 6 and rate >= 16000:
                from triggerfish_percussion.modulation_signature import (
                    modulation_signature,
                )
                from .ridge_contrast import ridge_contrast

                result[name].update(
                    motion=modulation_signature(audio, rate),
                    ridges=ridge_contrast(audio, rate),
                )
            write_wav(output / (name + ".wav"), AudioBuffer(audio, rate))
            if name == "candidate":
                result["performance"] = performance(saved, seconds, output)
    write_json(output / "audit.json", result)
    return result


def performance(saved, seconds, output):
    """Velocity probes are robustness tests, not matches to the reference gesture."""
    cases = {f"velocity-{s}": [dict(time=0, strength=s)] for s in (0.3, 0.6, 0.9)}
    for interval in (0.5, 0.125):
        cases[f"repeated-{interval}"] = [dict(time=i * interval) for i in range(5)]
    rows = []
    for name, hits in cases.items():
        audio = saved.sequence(saved.initial, max(seconds, 3), hits)
        if not np.isfinite(audio).all():
            raise ValueError("Non-finite performance render")
        rows.append(
            dict(
                name=name,
                hits=hits,
                peak_dbfs=float(20 * np.log10(max(1e-20, np.max(abs(audio))))),
                energy=float(audio @ audio),
            )
        )
        write_wav(output / (name + ".wav"), AudioBuffer(audio, saved.sample_rate))
    return rows


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    audit(args.directory, args.reference, args.output)
