"""C callers receive the current error, not an unrelated earlier failure."""

import ctypes as ct

import pytest

from drumfoundry import Renderer, default_patch
from drumfoundry._native import Strike


@pytest.mark.parametrize("operation", ["reset", "event", "process", "mute", "trigger"])
def test_errors_are_current_and_success_clears_them(operation):
    with Renderer(default_patch()) as r:
        lib, handle = r._lib, r._handle
        event = Strike()
        assert lib.df_default_strike(handle, ct.byref(event))
        calls = {
            "reset": lambda p: lib.df_reset(p),
            "event": lambda p: lib.df_default_strike(p, ct.byref(event)),
            "process": lambda p: lib.df_process(p, None, 0),
            "mute": lambda p: lib.df_set_mute(p, 0.5),
            "trigger": lambda p: lib.df_trigger(p, ct.byref(event)),
        }
        assert not lib.df_default_patch(b"bad-recipe")
        assert not calls[operation](None)
        assert lib.df_last_error() == b"Null native argument"
        assert calls[operation](handle)
        assert lib.df_last_error() == b""


def test_bad_mute_and_buffer_report_specific_failures():
    with Renderer(default_patch()) as r:
        lib, handle = r._lib, r._handle
        assert not lib.df_set_mute(handle, float("nan"))
        assert b"Mute must be finite" in lib.df_last_error()
        assert not lib.df_process(handle, None, 1)
        assert lib.df_last_error() == b"Null native argument"
        assert lib.df_process(handle, None, 0)
        assert lib.df_last_error() == b""


def test_truncated_unicode_error_stays_a_useful_native_error():
    patch = default_patch()
    patch["nodes"][0]["parameters"]["界" * 300] = 1
    with pytest.raises(ValueError, match="Unknown or misplaced parameter"):
        Renderer(patch)
