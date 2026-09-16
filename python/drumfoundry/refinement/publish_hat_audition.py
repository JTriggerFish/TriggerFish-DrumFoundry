"""Explicitly publish reviewed static-state artifacts into a fresh local folder."""

import argparse
from copy import deepcopy
from hashlib import sha256
import json
from pathlib import Path
from uuid import uuid4
from drumfoundry import Renderer
from drumfoundry.fitting import parameters
from .artifacts import write_json
from .hat_layers import STATES


def publish(run, destination):
    """Three main presets, plus reference-layer views of the same three sounds."""
    documents = {}
    shared_frequencies = None
    for state in STATES:
        files = sorted((run / state).glob("*.fit.json"))
        if len(files) != 4:
            raise ValueError(f"Expected four audited layers for {state}")
        report = json.loads((run / state / "report.json").read_text(encoding="utf8"))
        if report.get("exact_reload") is not True or len(report.get("layers", [])) != 4:
            raise ValueError("Missing successful reload audit")
        baseline = None
        for index, file in enumerate(files):
            if (
                sha256(file.read_bytes()).hexdigest()
                != report["layers"][index]["fit_sha256"]
            ):
                raise ValueError("Candidate changed since the recorded audit")
            with Renderer(file) as voice:
                fit = voice.document
            p = parameters(fit)
            if baseline is not None and p != baseline:
                raise ValueError("Audition layers do not share identical parameters")
            baseline = p
            geometry = {
                k: v for k, v in p.items() if k.startswith("resolved_frequency_")
            }
            if shared_frequencies is not None and geometry != shared_frequencies:
                raise ValueError("State geometries differ; review before publishing")
            shared_frequencies = geometry
            documents[Path("Velocity comparisons") / state / file.name] = fit
            if index == 2:
                main = deepcopy(fit)
                main.update(
                    id=str(uuid4()), parentId=fit["id"], name=f"Hi-hat — {state}"
                )
                documents[Path(f"{STATES.index(state)+1:02d}-{state}.fit.json")] = main
    destination.mkdir(parents=True, exist_ok=False)
    for path, fit in documents.items():
        (destination / path).parent.mkdir(parents=True, exist_ok=True)
        write_json(destination / path, fit)
    return list(documents)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    print(publish(args.run, args.destination))


if __name__ == "__main__":
    main()
