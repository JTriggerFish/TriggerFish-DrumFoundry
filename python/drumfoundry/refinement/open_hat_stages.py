"""Explicit continuation experiments; all bounds are visible and serialized."""


def stages_for(focus):
    """Select body, damping-only or the full continuation sequence."""
    stages = [
        (
            "decay",
            {
                "body_decay_seconds_0": (0.3, 7, True),
                "body_decay_seconds_7": (0.3, 5, True),
            },
        ),
        (
            "texture",
            {
                "field_turbulence": (0.3, 1.6, True),
                "field_packet_spread": (0.3, 5, True),
                "field_phase_bandwidth": (0.00002, 0.03, True),
                "field_phase_tilt": (-1, 1, False),
            },
        ),
        (
            "balance",
            {
                "body_brightness": (-8, 12, False),
                "body_excitation_centre": (200, 7000, True),
                "bloom_rate": (0, 6, False),
            },
        ),
        (
            "strike",
            {
                "impact_tone_noise": (0.1, 1, False),
                "impact_width": (0.25, 2, True),
                "direct_gain": (0, 1, False),
                "velocity_brightness": (0, 12, False),
            },
        ),
        (
            "decay-final",
            {
                "body_decay_seconds_0": (0.3, 7, True),
                "body_decay_seconds_7": (0.3, 5, True),
            },
        ),
    ]
    if focus == "body":
        stages = [
            (
                "coherence",
                {
                    "field_turbulence_slope": (-0.25, 1, False),
                    "field_phase_tilt": (-2, 1, False),
                    "field_phase_bandwidth": (0.00002, 0.03, True),
                },
            ),
            (
                "body",
                {
                    "body_tune": (0.95, 1.06, False),
                    "body_brightness": (-8, 12, False),
                    "bloom_rate": (0, 3, False),
                },
            ),
            (
                "decay-final",
                {
                    "body_decay_seconds_0": (0.3, 7, True),
                    "body_decay_seconds_7": (0.3, 5, True),
                },
            ),
        ]
    if focus == "decay-curve":
        stages = [
            (
                "decay-curve",
                {
                    "body_decay_seconds_0": (0.5, 5, True),
                    "body_decay_seconds_3": (0.5, 5, True),
                    "body_decay_seconds_7": (0.3, 4, True),
                },
            )
        ]
    return stages


INITIAL_STAGES = [
    (
        "spectral-balance",
        {
            "body_brightness": (-8, 14, False),
            "body_excitation_centre": (400, 7000, True),
            "bloom_rate": (0, 6, False),
        },
    ),
    (
        "decay",
        {
            "body_decay_seconds_0": (0.2, 5, True),
            "body_decay_seconds_7": (0.2, 5, True),
        },
    ),
    (
        "texture",
        {
            "field_turbulence": (0.2, 1.6, True),
            "field_packet_spread": (0.2, 4, True),
            "field_phase_bandwidth": (0.00002, 0.04, True),
            "field_phase_tilt": (-1, 1, False),
        },
    ),
    (
        "strike",
        {
            "impact_tone_noise": (0.1, 1, False),
            "impact_width": (0.25, 2, True),
            "direct_gain": (0, 1, False),
            "velocity_brightness": (0, 12, False),
        },
    ),
]
