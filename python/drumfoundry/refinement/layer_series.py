"""Explicit whole-series coordinates; never optimize individual painted modes."""

import math


def stretched_series(parameters, fundamental, stretch, count=24, core=3, level=-12):
    """Materialize the UI's protected-core harmonic law into ordinary parameters.

    No runtime macro/hidden state is introduced. Each handle has equal prominence,
    allocation and local turbulence; all decay shaping uses the global curve.
    """
    if not 1 <= count <= 32 or not 1 <= core <= 8 or not 0 <= stretch <= 1:
        raise ValueError("Invalid series shape")
    frequencies = [
        fundamental * n * math.hypot(1, stretch * max(0, (n - core) / core))
        for n in range(1, count + 1)
    ]
    if not all(math.isfinite(f) and 1 <= f <= 20000 for f in frequencies):
        raise ValueError("Series exceeds the modal editor range")
    result = dict(parameters)
    for i in range(32):
        result[f"resolved_frequency_{i}"] = frequencies[min(i, count - 1)]
        result[f"resolved_level_{i}"] = level if i < count else -72
        result[f"resolved_turbulence_{i}"] = 1
        result[f"resolved_allocation_{i}"] = 1
    for i in range(1, 7):
        result[f"body_decay_active_{i}"] = 0
    return result


def observation_headroom(parameters, boost_db=12):
    """Equivalent visible gain staging, leaving excitation and stored energy alone.

    Raise every active observation bar and direct observation gain together,
    compensating at model level. Subsequent level fitting then has upward room
    without requiring positive model dB or per-reference normalization.
    """
    result = dict(parameters)
    for i in range(32):
        key = f"resolved_level_{i}"
        if result[key] > -72:
            result[key] += boost_db
            if result[key] > 6:
                raise ValueError("Observation headroom exceeds modal level range")
    result["direct_gain"] *= 10 ** (boost_db / 20)
    result["model_level_db"] -= boost_db
    if result["direct_gain"] > 2 or result["model_level_db"] < -60:
        raise ValueError("Equivalent observation scaling exceeds native limits")
    return result
