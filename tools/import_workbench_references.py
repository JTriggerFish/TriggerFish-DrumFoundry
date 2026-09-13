"""One-time development import of the old workbench's curated local references.

The native application reads the resulting JSON/WAVs directly. This helper is
not an application dependency and never redistributes or modifies source audio.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
from urllib.parse import unquote


def import_references(legacy: Path, destination: Path) -> int:
    """Reuse the curated selector, then copy its exact allow-list unchanged."""
    source = legacy.resolve() / "tools/percussion_reference_corpus.py"
    spec = importlib.util.spec_from_file_location("legacy_reference_catalog", source)
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load the reference catalogue builder")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    private = legacy.resolve() / (
        "build/cymbal-calibration/references/"
        "private-corpus-a-crash-v1/cells-oh-dyn-v2"
    )
    corpora, paths = module.build_catalog(private)
    root = destination.resolve()
    root.mkdir(parents=True, exist_ok=True)
    count = 0
    for corpus in corpora:
        for cell in corpus["cells"]:
            source_path = paths[unquote(cell["url"])]
            target = root / corpus["id"] / source_path.name
            target.resolve().relative_to(root)
            with source_path.open("rb") as source_audio:
                digest = hashlib.file_digest(source_audio, "sha256").hexdigest()
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                with target.open("rb") as existing:
                    if hashlib.file_digest(existing, "sha256").hexdigest() != digest:
                        raise RuntimeError(f"Existing reference differs: {target}")
            else:
                with source_path.open("rb") as src, target.open("xb") as dst:
                    shutil.copyfileobj(src, dst)
            cell["path"] = target.relative_to(root).as_posix()
            cell["sha256"] = digest
            count += 1
    document = {"schema": "triggerfish.drumfoundry.references/v1", "corpora": corpora}
    output = root / "catalog.json"
    if output.exists():
        if json.loads(output.read_text(encoding="utf-8")) != document:
            raise RuntimeError("Existing catalogue differs; choose a new destination")
    else:
        with output.open("x", encoding="utf-8") as stream:
            json.dump(document, stream, indent=2)
            stream.write("\n")
    return count


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("legacy", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    count = import_references(args.legacy, args.destination)
    print(f"Imported {count} references into {args.destination}")


if __name__ == "__main__":
    main()
