"""Prepare separately auditionable hi-hat states from a local reference catalog."""

from copy import deepcopy
from .reference import load_reference, reference_attachment
from .layer_loss import LayerLoss
from .layer_series import stretched_series

STATES = ("closed", "half-open", "open")


def starting_parameters(parameters, state):
    """Same authored modal structure; only starting damping differs by state."""
    p = stretched_series(parameters, 260, 0.22)
    low, high = {"closed": (0.3, 0.12), "half-open": (1.3, 0.8), "open": (2.3, 1.3)}[
        state
    ]
    p.update(
        body_decay_seconds_0=low,
        body_decay_seconds_7=high,
        body_excitation=1,
        field_gain=1.6,
        model_level_db=-12,
        direct_gain=0.06,
        output_eq_enabled=0,
        body_tune=1,
        bloom_rate=0.4,
        body_brightness=4,
        body_excitation_centre=2500,
        field_turbulence=2,
        field_turbulence_slope=0.2,
        field_packet_spread=2,
        field_satellite_density=1,
        field_distribution=1,
        field_phase_bandwidth=0.035,
        field_phase_tilt=0.5,
        field_motion_depth=0.15,
        field_motion_rate=35,
        field_motion_sharing=0.25,
        field_wander_hz=0,
        field_beat_depth=0.12,
        field_doublet_split=1.25,
        impact_tone_noise=0.9,
        impact_width=0.7,
        impact_noise_tilt=0,
        velocity_brightness=4,
    )
    return p


def load_layers(source, catalog, root, state, rate=44100, level_policy="absolute"):
    """Keep catalog strength provisional and fixed; never infer gain per layer."""
    corpus = next(c for c in catalog["corpora"] if c["id"] == "hihat-14-reference")
    cells = sorted(
        (c for c in corpus["cells"] if c["articulation"] == state),
        key=lambda c: c["velocity"],
    )
    if len(cells) < 3:
        raise ValueError(f"Need at least three layers for {state}")
    result = []
    for cell in cells:
        fit = deepcopy(source)
        fit["reference"] = {"sha256": cell["sha256"]}
        path = root / cell["path"]
        ref = load_reference(
            fit, path, rate, gain_db=8, onset=cell["onset_seconds"], channel=0
        )
        event = {
            k: cell[k]
            for k in ("strength", "location", "hardness", "implement", "contactSpread")
        }
        # Static state fits use only explicit T60 damping, not an extra mute.
        event["constraint"] = 0
        attachment = reference_attachment(fit, ref, path, root)
        attachment.update(name=cell["label"], visible=True)
        result.append(
            dict(
                reference=ref,
                loss=LayerLoss(ref.window(3), rate, level_policy),
                attachment=attachment,
                event=event,
                cell=cell,
            )
        )
    return result
