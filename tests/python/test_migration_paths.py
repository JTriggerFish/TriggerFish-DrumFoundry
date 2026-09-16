"""The migration CLI and signature tests must use the same explicit archive."""

import hashlib
import json
import subprocess
import sys
from pathlib import Path

import pytest

from drumfoundry import Renderer
from drumfoundry.migration import render_case, sound_identity

ROOT = Path(__file__).resolve().parents[2]


def test_cli_reads_manifest_archive(tmp_path):
    """Exercise CLI lookup/hash validation without depending on private PCM."""
    factory = tmp_path / "factory"
    legacy = tmp_path / "legacy"
    oracles = tmp_path / "oracles"
    for folder in (factory, legacy, oracles):
        folder.mkdir()
    old = json.loads((ROOT / "presets/legacy/hihat.fit.json").read_text())
    (legacy / "hihat.fit.json").write_text(json.dumps(old))
    # The factory must not be read, even when it exists at the old name.
    (factory / "hihat.fit.json").write_text("not the historical preset")
    with Renderer(old, 44100) as renderer:
        pcm = renderer.render(6).astype("<f4").tobytes()
    (oracles / "hihat-44100-false.f32").write_bytes(pcm)
    case = dict(
        preset="hihat",
        preset_file="../legacy/hihat.fit.json",
        rate=44100,
        repeated=False,
        sound_sha256=sound_identity(old),
        oracle_sha256=hashlib.sha256(pcm).hexdigest(),
    )
    baseline = tmp_path / "baseline.json"
    baseline.write_text(json.dumps({"cases": [case]}))
    command = [
        sys.executable,
        "-m",
        "drumfoundry.migration",
        str(baseline),
        str(factory),
        str(oracles),
    ]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    assert json.loads(result.stdout)[0]["relative_rms"] == 0
    # Never substitute a newer sound or regenerate the expected identity.
    old["controls"]["event"]["strength"] *= 0.5
    (legacy / "hihat.fit.json").write_text(json.dumps(old))
    with pytest.raises(ValueError, match="Migration preset changed"):
        render_case(factory, case)
    (legacy / "hihat.fit.json").unlink()
    with pytest.raises(FileNotFoundError):
        render_case(factory, case)


def test_historical_hat_cases_explicitly_declare_archive():
    baseline = json.loads((ROOT / "tests/fixtures/migration-v1.json").read_text())
    cases = [c for c in baseline["cases"] if c["preset"] == "hihat"]
    assert len(cases) == 4
    assert all(c["preset_file"] == "../legacy/hihat.fit.json" for c in cases)
