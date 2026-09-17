"""Product metadata must agree across the package and loaded native engine."""

from importlib.metadata import version
from pathlib import Path

from drumfoundry import native_version


def test_product_version():
    expected = (Path(__file__).resolve().parents[2] / "VERSION.txt").read_text().strip()
    assert native_version() == expected
    assert version("triggerfish-drumfoundry") == expected
