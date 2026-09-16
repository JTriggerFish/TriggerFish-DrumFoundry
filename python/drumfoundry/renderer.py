"""Owned native voices with explicit reset, retrigger and offline render semantics."""

import ctypes as ct
import json
import math
from pathlib import Path

import numpy as np

from ._native import Strike, checked, load_library


def encode(document):
    """Paths and JSON objects are accepted; strings are treated as raw JSON."""
    if isinstance(document, Path):
        return document.read_bytes()
    if isinstance(document, str):
        return document.encode("utf8")
    return json.dumps(document, allow_nan=False).encode("utf8")


def default_patch(recipe="metal.cymbal.v1"):
    """Create a complete patch using native parameter defaults and routing."""
    lib = load_library()
    return json.loads(checked(lib, lib.df_default_patch(recipe.encode("utf8"))))


class Renderer:
    """One independent engine. Do not call the same renderer concurrently.

    Live-style process/trigger preserves stored energy. render() resets first.
    configure() prepares a fresh voice off-thread; it is not a realtime edit.
    All PCM is raw model output: no limiter, normalization or monitoring gain.
    """

    def __init__(self, document, sample_rate=48000):
        self._handle = None
        self._lib = load_library()
        self.sample_rate = float(sample_rate)
        self._handle = checked(
            self._lib, self._lib.df_create(self.sample_rate, encode(document))
        )

    def _require_open(self):
        if self._handle is None:
            raise RuntimeError("Renderer is closed")

    def close(self):
        if self._handle is not None:
            self._lib.df_destroy(self._handle)
            self._handle = None

    def __enter__(self):
        self._require_open()
        return self

    def __exit__(self, *_):
        self.close()

    def __del__(self):
        self.close()

    @property
    def document(self):
        self._require_open()
        return json.loads(checked(self._lib, self._lib.df_document(self._handle)))

    @property
    def descriptors(self):
        self._require_open()
        return json.loads(checked(self._lib, self._lib.df_descriptors(self._handle)))

    def configure(self, document):
        """Load a full patch/fit; rejection leaves the existing voice untouched."""
        self._require_open()
        checked(self._lib, self._lib.df_configure(self._handle, encode(document)))

    def reset(self):
        self._require_open()
        checked(self._lib, self._lib.df_reset(self._handle))

    def _strike(self, overrides):
        event = Strike()
        checked(self._lib, self._lib.df_default_strike(self._handle, ct.byref(event)))
        known = {key for key, _ in Strike._fields_}
        if overrides.keys() - known:
            raise ValueError(f"Unknown strike controls: {overrides.keys() - known}")
        for key, value in overrides.items():
            if key == "seed":
                if (
                    isinstance(value, bool)
                    or int(value) != value
                    or not 0 <= value <= 0xFFFFFFFF
                ):
                    raise ValueError("Invalid strike seed")
                value = int(value)
            elif not math.isfinite(value) or not 0 <= value <= 1:
                raise ValueError(f"Invalid strike control: {key}")
            setattr(event, key, value)
        return event

    def trigger(self, **event):
        self._require_open()
        strike = self._strike(event)
        checked(self._lib, self._lib.df_trigger(self._handle, ct.byref(strike)))

    def set_mute(self, amount):
        """Damp the current metallic voice without resetting or retriggering it."""
        self._require_open()
        if not math.isfinite(amount) or not 0 <= amount <= 1:
            raise ValueError("Mute must be between zero and one")
        checked(self._lib, self._lib.df_set_mute(self._handle, amount))

    def set_parameter(self, key, value):
        """Live scalar edit, preserving ringing state; source document is unchanged.

        Uses the native host-automation path, including validation and smoothing.
        Geometry changes needing preparation are rejected, not silently reset.
        """
        self._require_open()
        if not math.isfinite(value):
            raise ValueError("Parameter value must be finite")
        for index, descriptor in enumerate(self.descriptors):
            if descriptor["key"] == key:
                checked(
                    self._lib, self._lib.df_set_parameter(self._handle, index, value)
                )
                return
        raise ValueError(f"Unknown parameter: {key}")

    def process(self, frames):
        """Render into a NumPy-owned float32 buffer in one native call."""
        self._require_open()
        if (
            isinstance(frames, bool)
            or int(frames) != frames
            or not 0 <= frames <= 0xFFFFFFFF
        ):
            raise ValueError("Invalid frame count")
        result = np.empty(int(frames), dtype=np.float32)
        checked(
            self._lib,
            self._lib.df_process(
                self._handle, result.ctypes.data_as(ct.POINTER(ct.c_float)), int(frames)
            ),
        )
        return result

    def render(self, seconds, events=None):
        """Reset then render timed hits; None means one saved strike at t=0.

        An explicit empty list renders silence. Hits at the same frame retain
        input order. Timing uses nearest sample, with half samples rounded up.
        """
        if not math.isfinite(seconds) or seconds <= 0 or seconds > 120:
            raise ValueError("Render duration must be in (0, 120] seconds")
        frames = math.floor(seconds * self.sample_rate + 0.5)
        plan, previous = [], 0
        self._require_open()
        for hit in ([{"time": 0}] if events is None else events):
            time = hit["time"]
            if not math.isfinite(time) or time < 0:
                raise ValueError("Invalid hit time")
            frame = math.floor(time * self.sample_rate + 0.5)
            if frame < previous or frame >= frames:
                raise ValueError("Hits must be ordered and within the render")
            event = self._strike(
                {key: value for key, value in hit.items() if key != "time"}
            )
            plan.append((frame, event))
            previous = frame
        self.reset()
        result, cursor = np.empty(frames, dtype=np.float32), 0
        for frame, event in plan:
            result[cursor:frame] = self.process(frame - cursor)
            checked(self._lib, self._lib.df_trigger(self._handle, ct.byref(event)))
            cursor = frame
        result[cursor:] = self.process(frames - cursor)
        return result
