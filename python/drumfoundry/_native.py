"""Explicit C ABI declarations; no C++ ABI crosses the Python boundary."""

import ctypes as ct
import os
from pathlib import Path
import sys


class Strike(ct.Structure):
    _fields_ = [
        (key, ct.c_float)
        for key in (
            "strength",
            "location",
            "hardness",
            "implement",
            "contactSpread",
            "constraint",
        )
    ] + [("seed", ct.c_uint32)]


class Series(ct.Structure):
    _fields_ = [
        (key, ct.c_double)
        for key in (
            "fundamental",
            "stretch",
            "level",
            "rolloff",
            "turbulence",
            "minimum",
            "maximum",
        )
    ] + [
        (key, ct.c_uint32)
        for key in ("count", "harmonic_core", "first", "family", "truncate_to_range")
    ]


class Mode(ct.Structure):
    _fields_ = [
        (key, ct.c_double) for key in ("frequency", "level", "turbulence", "allocation")
    ]


def library_path():
    """Resolve an explicit override or this checkout's native build."""
    if os.environ.get("DRUMFOUNDRY_LIBRARY"):
        return Path(os.environ["DRUMFOUNDRY_LIBRARY"]).resolve()
    suffix = (
        ".dll"
        if sys.platform == "win32"
        else ".dylib" if sys.platform == "darwin" else ".so"
    )
    return (
        Path(__file__).resolve().parents[2]
        / "build/native"
        / f"drumfoundry_native{suffix}"
    )


def load_library():
    """Load only when constructing a renderer; importing analysis needs no DLL."""
    path = library_path()
    if not path.is_file():
        raise RuntimeError(f"Native library not found: {path}. Run ./dev.ps1 build.")
    lib = ct.CDLL(str(path))
    pointer, string, integer = ct.c_void_p, ct.c_char_p, ct.c_int
    signatures = {
        "df_last_error": (string, []),
        "df_generate_series": (
            integer,
            [ct.POINTER(Series), ct.POINTER(Mode), ct.POINTER(ct.c_uint32)],
        ),
        "df_transform_series": (
            integer,
            [
                ct.POINTER(ct.c_double),
                ct.c_uint32,
                ct.c_double,
                ct.c_double,
                ct.c_uint32,
                ct.c_double,
                ct.c_double,
                ct.POINTER(ct.c_double),
            ],
        ),
        "df_create": (pointer, [ct.c_float, string]),
        "df_destroy": (None, [pointer]),
        "df_configure": (integer, [pointer, string]),
        "df_document": (string, [pointer]),
        "df_descriptors": (string, [pointer]),
        "df_default_patch": (string, [string]),
        "df_reset": (integer, [pointer]),
        "df_set_mute": (integer, [pointer, ct.c_float]),
        "df_default_strike": (integer, [pointer, ct.POINTER(Strike)]),
        "df_trigger": (integer, [pointer, ct.POINTER(Strike)]),
        "df_process": (integer, [pointer, ct.POINTER(ct.c_float), ct.c_uint32]),
    }
    for name, (result, args) in signatures.items():
        fn = getattr(lib, name)
        fn.restype, fn.argtypes = result, args
    return lib


def checked(lib, result):
    if not result:
        message = lib.df_last_error()
        raise ValueError(
            message.decode("utf8", errors="replace")
            if message
            else "Native operation failed"
        )
    return result
