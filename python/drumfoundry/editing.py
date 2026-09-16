"""Native design-time helpers shared with the editor; no Python synthesis laws."""

import ctypes as ct
from functools import lru_cache
from ._native import Series, Mode, checked, load_library


@lru_cache(maxsize=1)
def _library():
    return load_library()


class SeriesRangeError(ValueError):
    """An otherwise valid transform would move modes outside the allowed range."""


def transform_series(
    frequencies, pitch=1, stretch=0, *, core=4, minimum=1, maximum=20000
):
    """Native saved-series deformation, preserving slots and rejecting overflow."""
    frequencies = list(frequencies)
    if (
        isinstance(core, bool)
        or int(core) != core
        or not 1 <= core <= 8
        or len(frequencies) > 32
    ):
        raise ValueError("Invalid series transform size/core")
    values = (ct.c_double * len(frequencies))(*frequencies)
    output = (ct.c_double * len(frequencies))()
    lib = _library()
    status = lib.df_transform_series(
        values, len(frequencies), pitch, stretch, int(core), minimum, maximum, output
    )
    checked(lib, status)
    if status == 2:
        raise SeriesRangeError("Transformed modes exceed the frequency range")
    return list(output)


def generate_series(
    fundamental,
    stretch=0,
    *,
    count=16,
    core=4,
    first=1,
    level=0,
    rolloff=6,
    turbulence=1,
    minimum=1,
    maximum=20000,
    family="harmonic",
    truncate=False,
):
    """Return native mode dictionaries. Overflow rejects unless explicitly truncated.

    This is a design-time operation; importing this module does not load a DLL.
    Integer checks prevent ctypes from silently wrapping malformed arguments.
    """
    for value in (count, core, first):
        if isinstance(value, bool) or int(value) != value or not 0 <= value <= 32:
            raise ValueError("Invalid series integer")
    if family not in ("harmonic", "membrane") or not isinstance(truncate, bool):
        raise ValueError("Invalid series option")
    settings = Series(
        fundamental,
        stretch,
        level,
        rolloff,
        turbulence,
        minimum,
        maximum,
        int(count),
        int(core),
        int(first),
        int(family == "membrane"),
        int(truncate),
    )
    output, length = (Mode * 32)(), ct.c_uint32()
    lib = _library()
    checked(lib, lib.df_generate_series(ct.byref(settings), output, ct.byref(length)))
    return [
        {key: getattr(mode, key) for key, _ in Mode._fields_}
        for mode in output[: length.value]
    ]
