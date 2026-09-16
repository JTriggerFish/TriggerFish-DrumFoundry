"""The Python design surface delegates to the native editor generator."""

import math
import pytest
from drumfoundry.editing import SeriesRangeError, generate_series, transform_series
from drumfoundry.refinement.fit_open_hat import series


def test_native_series_protected_core_and_partial_offset():
    modes = generate_series(110, 0.2, count=10, core=3, rolloff=0)
    assert [m["frequency"] for m in modes[:3]] == [110, 220, 330]
    assert modes[5]["frequency"] == pytest.approx(660 * math.hypot(1, 0.2))
    assert generate_series(110, 0.2, count=7, core=3, first=4, rolloff=0) == modes[3:]


def test_native_series_membrane_and_range_policy():
    modes = generate_series(100, family="membrane", count=3)
    assert modes[1]["frequency"] == pytest.approx(159.3340506)
    with pytest.raises(ValueError):
        generate_series(1000, count=32)
    assert len(generate_series(1000, count=32, truncate=True)) == 20
    # The hat path no longer silently discards the 15--20 kHz range.
    p = series({}, 1000, 0)
    assert p["resolved_frequency_19"] == 20000
    assert p["resolved_level_19"] > -72
    assert p["resolved_level_20"] == -72


@pytest.mark.parametrize(
    "options",
    [
        dict(count=0),
        dict(count=33),
        dict(count=2.5),
        dict(core=0),
        dict(first=-1),
        dict(first=33),
        dict(stretch=-0.1),
        dict(stretch=float("nan")),
        dict(family="unknown"),
        dict(truncate=2),
        dict(family="membrane", first=32, count=2),
    ],
)
def test_native_series_rejects_invalid_arguments(options):
    with pytest.raises(ValueError):
        generate_series(100, **options)


def test_saved_transform_shares_generator_law_and_preserves_slot_order():
    original = [600, 100, 400, 300, 500, 200]
    result = transform_series(original, 1.1, 0.2, core=3)
    expected = generate_series(110, 0.2, core=3, count=6)
    assert result == pytest.approx(
        [expected[int(f / 100) - 1]["frequency"] for f in original]
    )
    assert transform_series(result, 1 / 1.1, -0.2, core=3) == pytest.approx(original)
    assert transform_series(original) == original
    assert transform_series([]) == []


def test_saved_transform_rejects_whole_candidate_without_clamping():
    frequencies = [100, 200, 300, 18000, 19000]
    with pytest.raises(SeriesRangeError):
        transform_series(frequencies, 1.15, 0.075, core=3)
    with pytest.raises(SeriesRangeError):
        transform_series([1, 200], 0.9)
    assert frequencies == [100, 200, 300, 18000, 19000]


@pytest.mark.parametrize(
    "frequencies,options",
    [
        ([0], {}),
        ([float("nan")], {}),
        ([20001], {}),
        ([100] * 33, {}),
        ([100], dict(pitch=0)),
        ([100], dict(stretch=2)),
        ([100], dict(core=1.5)),
        ([100], dict(maximum=float("inf"))),
    ],
)
def test_saved_transform_validation_is_not_range_rejection(frequencies, options):
    with pytest.raises(ValueError) as error:
        transform_series(frequencies, **options)
    assert not isinstance(error.value, SeriesRangeError)
