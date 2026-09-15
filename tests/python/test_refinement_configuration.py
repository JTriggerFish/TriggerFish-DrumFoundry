"""Reject misleading experiment contracts before writing or rendering anything."""

import json
from types import SimpleNamespace
import pytest

from drumfoundry.refinement.configuration import validated
from drumfoundry.refinement.reference import Reference, reference_attachment
from drumfoundry.refinement.metal_search import damping_cases


def options(**updates):
    return SimpleNamespace(
        **dict(
            dict(
                profile="metal",
                seconds=6,
                budget=1,
                stage="locked-texture",
                plots=False,
            ),
            **updates,
        )
    )


@pytest.mark.parametrize(
    "changes",
    [
        dict(seconds=1),
        dict(budget=-1),
        dict(budget=0.5),
        dict(stage="gong-grid", budget=0),
        dict(solver="unknown"),
        dict(profile="kick", seconds=1.2, stage="audit-only", plots=True),
    ],
)
def test_reject_invalid_contract(changes):
    with pytest.raises(ValueError):
        validated(options(**changes))


def test_bounds_are_frozen_and_input_unchanged(tmp_path):
    path = tmp_path / "bounds.json"
    bounds = {"body_tune": [0.5, 2]}
    path.write_text(json.dumps(bounds))
    args = options(stage="coordinates", bounds=path, seconds=None)
    result = validated(args)
    path.write_text("{}")
    assert result.coordinate_bounds == bounds
    assert result.seconds == 6
    assert args.seconds is None


def test_unverified_attachment_needs_library_root(tmp_path):
    ref = Reference(
        None, 48000, dict(sha256="new", gain_db=0, onset_seconds=0, channel=0)
    )
    fit = {"reference": {"libraryPath": "old.wav"}}
    with pytest.raises(ValueError, match="unverified"):
        reference_attachment(fit, ref, tmp_path / "new.wav")
    attachment = reference_attachment(fit, ref, tmp_path / "new.wav", tmp_path)
    assert attachment["libraryPath"] == "new.wav"
    assert attachment["sha256"] == "new"


def test_sparse_decay_trials_respect_minimum():
    base = {"body_decay_seconds_0": 0.02, "body_decay_seconds_7": 0.02}
    for _, trial in damping_cases(base):
        assert trial["body_decay_seconds_7"] >= 0.02
