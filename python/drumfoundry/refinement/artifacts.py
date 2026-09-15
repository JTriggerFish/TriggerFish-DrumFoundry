"""Exclusive frozen-fit artifacts and optional fixed-scale inspection plots."""

import json
from hashlib import sha256
from pathlib import Path
from triggerfish_percussion.audio_io import AudioBuffer, write_wav


def write_json(path, value):
    """Exclusive output: a fit/audit never overwrites an existing artifact."""
    with Path(path).open("x", encoding="utf8") as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write("\n")


def export(args, saved, candidate, report, rows, target, baseline, best):
    """Write one complete run; the caller has reserved a fresh output directory."""
    write_json(args.output / "source.fit.json", saved.fit)
    write_json(args.output / "candidate.fit.json", candidate)
    report["fits_sha256"] = {
        name: sha256((args.output / file).read_bytes()).hexdigest()
        for name, file in (
            ("before", "source.fit.json"),
            ("candidate", "candidate.fit.json"),
        )
    }
    write_json(args.output / "report.json", report)
    write_json(args.output / "progress.json", rows)
    signals = {
        "Reference": target,
        "Before": baseline,
        "Candidate": saved.render(best["parameters"], args.seconds),
    }
    for name, audio in signals.items():
        write_wav(args.output / (name.lower() + ".wav"), AudioBuffer(audio, args.rate))
    if args.plots and args.profile == "metal":
        from .plots import plots, spectrograms

        plots(signals, args.rate, args.output)
        spectrograms(target, signals["Candidate"], args.rate, args.output)
