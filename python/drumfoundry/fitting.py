"""Bounded, explicit-coordinate fitting against the exact native patch renderer.

This is a transport-independent search driver, not a new perceptual objective.
Pass one of the tested analysis losses; acceptance still requires diagnostics
and audition. No implicit EQ, gain matching, mode insertion or decay edits.
"""

from copy import deepcopy
from datetime import datetime, timezone
import hashlib
import uuid

import numpy as np
from scipy.optimize import minimize

from ._native import library_path


def parameters(document):
    """Return all explicit node values in a fit or patch."""
    patch = document.get("instrument", document)
    return {
        key: value
        for node in patch["nodes"]
        for key, value in node["parameters"].items()
    }


def with_parameters(document, updates):
    """Copy a document and edit existing values without changing ownership."""
    result = deepcopy(document)
    remaining = set(updates)
    for node in result.get("instrument", result)["nodes"]:
        for key in node["parameters"].keys() & updates.keys():
            node["parameters"][key] = float(updates[key])
            remaining.discard(key)
    if remaining:
        raise ValueError(
            f"Parameters are not explicitly stored in the patch: {remaining}"
        )
    return result


def _coordinates(renderer, document, bounds):
    """Validate the selected continuous coordinates before touching the voice."""
    original = parameters(document)
    descriptors = {d["key"]: d for d in renderer.descriptors}
    keys = list(bounds)
    low, high = np.array([bounds[k] for k in keys], dtype=float).T
    if not np.isfinite([low, high]).all() or np.any(low >= high):
        raise ValueError("Invalid search bounds")
    for k, lo, hi in zip(keys, low, high):
        d = descriptors[k]
        # Match native float endpoint validation without broad epsilon clamps.
        if (
            d["scale"] >= 2
            or abs(lo) > np.finfo(np.float32).max
            or abs(hi) > np.finfo(np.float32).max
            or np.float32(lo) < d["minimum"]
            or np.float32(hi) > d["maximum"]
        ):
            raise ValueError(f"Invalid continuous search coordinate: {k}")
        if not np.float32(lo) <= np.float32(original[k]) <= np.float32(hi):
            raise ValueError(f"Starting value outside bounds: {k}")
    start = np.clip((np.array([original[k] for k in keys]) - low) / (high - low), 0, 1)
    return keys, low, high, start


def _score(objective, audio):
    """Keep scalar losses native; convert vector residuals to mean square."""
    residual = np.asarray(objective(audio), dtype=float)
    if not residual.size or not np.isfinite(residual).all():
        raise ValueError("Objective returned a non-finite or empty result")
    return float(np.mean(residual * residual)) if residual.ndim else float(residual)


def _report(renderer, reference, bounds, objective, trace, best):
    owner = getattr(objective, "__self__", objective)
    return {
        "method": "bounded-powell-native-v1",
        "objective": getattr(
            owner,
            "specification",
            {
                "name": getattr(objective, "__name__", "custom"),
                "specification": "caller supplied; undeclared",
            },
        ),
        "bounds": bounds,
        "seed_policy": "saved event, fixed throughout this search",
        "sample_rate": renderer.sample_rate,
        "frames": len(reference),
        "reference_pcm_sha256": hashlib.sha256(
            reference.astype("<f4").tobytes()
        ).hexdigest(),
        "native_library_sha256": hashlib.sha256(
            library_path().read_bytes()
        ).hexdigest(),
        "baseline_score": trace[0]["score"],
        "best_score": best["score"],
        "trace": trace,
    }


def fit_parameters(renderer, reference, bounds, objective, *, budget=60):
    """Search named coordinates and return a candidate/trace, never publish it.

    objective(samples) returns a scalar or residual vector with fixed reference
    floors/weights. Saved gesture, seed and unselected parameters are unchanged.
    Restore the renderer's configuration afterward (not its in-flight tail).
    """
    reference = np.asarray(reference, dtype=np.float64)
    if reference.ndim != 1 or not reference.size or not np.isfinite(reference).all():
        raise ValueError("Reference must be finite, nonempty mono PCM")
    if not bounds or isinstance(budget, bool) or int(budget) != budget or budget < 1:
        raise ValueError("Provide coordinates and a positive evaluation budget")
    document = renderer.document
    keys, low, high, start = _coordinates(renderer, document, bounds)
    trace, best = [], {"score": float("inf"), "document": document}

    def evaluate(x):
        updates = dict(zip(keys, low + np.asarray(x) * (high - low)))
        candidate = with_parameters(document, updates)
        renderer.configure(candidate)
        audio = renderer.render(len(reference) / renderer.sample_rate)
        score = _score(objective, audio)
        trace.append(
            {"evaluation": len(trace) + 1, "score": score, "parameters": updates}
        )
        if score < best["score"]:
            best.update(score=score, document=candidate)
        return score

    try:
        evaluate(start)
        if budget > 1:
            minimize(
                evaluate,
                start,
                method="Powell",
                bounds=[(0, 1)] * len(keys),
                options={"maxfev": budget - 1, "xtol": 1e-4, "ftol": 1e-5},
            )
    finally:
        renderer.configure(document)
    candidate = deepcopy(best["document"])
    candidate.update(
        parentId=document.get("id"),
        id=str(uuid.uuid4()),
        createdAt=datetime.now(timezone.utc).isoformat(),
        name=document["name"] + " — candidate",
    )
    return candidate, _report(renderer, reference, bounds, objective, trace, best)
