"""Python uses the exact native scalar automation path for pedal experiments."""

import numpy as np
import pytest
from drumfoundry import Renderer
from drumfoundry.renderer import default_patch


def test_live_pedal_and_rejection():
    with Renderer(default_patch()) as voice:
        voice.set_parameter("hat_contact_enabled", 1)
        voice.process(512)
        voice.set_parameter("hat_openness", 0)
        chick = voice.process(24000)
        assert np.isfinite(chick).all() and np.max(np.abs(chick)) > 1e-6
        voice.reset()
        assert not np.any(voice.process(512))
        voice.set_parameter("hat_openness", 1)
        voice.process(512)
        voice.set_parameter("hat_openness", 0)
        voice.reset()
        assert not np.any(voice.process(512))
        voice.trigger(strength=0.6)
        assert np.any(voice.process(512))
        for key, value in [
            ("hat_openness", 2),
            ("missing", 0),
            ("hat_openness", float("nan")),
            ("body_tune", 1),
        ]:
            with pytest.raises(ValueError):
                voice.set_parameter(key, value)


def test_bypass_preserves_factory_sound():
    with Renderer(default_patch()) as a, Renderer(default_patch()) as b:
        b.set_parameter("hat_openness", 0.1)
        b.set_parameter("hat_clearance", 0.0001)
        b.set_parameter("hat_rattle_motion", 1)
        b.set_parameter("hat_settling", 1)
        b.process(512)
        np.testing.assert_array_equal(a.render(0.25), b.render(0.25))


def test_rattle_live_state_and_reset():
    with Renderer(default_patch()) as voice:
        for key, value in {
            "hat_contact_enabled": 1,
            "hat_rattle_motion": 0.7,
            "hat_settling": 0.8,
            "hat_clearance": 0.0001,
        }.items():
            voice.set_parameter(key, value)
        voice.reset()
        assert not np.any(voice.process(24000))
        voice.trigger(strength=0.6)
        before = voice.process(12000)
        voice.set_parameter("hat_rattle_motion", 0.3)
        voice.set_parameter("hat_settling", 0.4)
        after = voice.process(12000)
        assert np.isfinite(after).all() and np.max(np.abs(after)) > 1e-8
        assert np.max(np.abs(before)) > 1e-8
        voice.set_parameter("hat_openness", 0)
        assert np.isfinite(voice.process(12000)).all()
        voice.reset()
        assert not np.any(voice.process(24000))


@pytest.mark.parametrize("live", [False, True])
def test_settling_zero_is_slow_not_closed(live):
    """Neighbouring values must not switch contact model or discard a tail."""
    signals = []
    for settling in (0, 0.001):
        with Renderer(default_patch()) as voice:
            for key, value in {
                "hat_contact_enabled": 1,
                "hat_openness": 1,
                "hat_rattle_motion": 0.7,
                "hat_settling": 0.8 if live else settling,
                "hat_clearance": 0.0004,
                "hat_contact_loss": 0.62,
            }.items():
                voice.set_parameter(key, value)
            voice.reset()
            voice.trigger(strength=0.6)
            if live:
                voice.process(12000)
                voice.set_parameter("hat_settling", settling)
            signals.append(voice.process(48000))
    assert all(np.isfinite(x).all() for x in signals)
    # Impacts may shift in time; compare energy rather than exact waveforms.
    for a, b in zip(np.array_split(signals[0], 4), np.array_split(signals[1], 4)):
        ratio = np.sum(a * a) / np.sum(b * b)
        assert 0.8 < ratio < 1.25
