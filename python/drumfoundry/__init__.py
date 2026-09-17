"""Direct native DrumFoundry rendering. No browser, Node or Wasm transport."""

from .renderer import Renderer, default_patch


def native_version():
    """Report the loaded C++ engine version; load it only when requested."""
    from ._native import load_library

    return load_library().df_version().decode("ascii")


__all__ = ["Renderer", "default_patch", "native_version"]
