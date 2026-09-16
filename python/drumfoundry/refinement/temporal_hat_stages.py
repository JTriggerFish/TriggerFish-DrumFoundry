"""Explicit experiment bounds, separate from scheduling and the loss."""

STAGES = {
    "tonal-body": {
        "low_prominence": (-4, 6, False),
        "resolved_turbulence_0": (0, 1, False),
        "field_motion_depth": (0.2, 2, False),
        "field_motion_rate": (20, 200, True),
        "body_decay_seconds_0": (0.2, 8, True),
        "body_decay_seconds_7": (0.2, 6, True),
        "body_brightness": (-24, 16, False),
        "bloom_rate": (0.005, 16, True),
    },
    "body-bloom-decay": {
        "prominence_tilt": (-6, 6, False),
        "body_brightness": (-24, 12, False),
        "body_excitation_centre": (300, 12000, True),
        "bloom_rate": (0.005, 16, True),
        "bloom_energy_acceleration": (0, 1, False),
        "bloom_energy_sensitivity": (0, 2, False),
        "body_decay_seconds_0": (0.2, 8, True),
        "body_decay_seconds_7": (0.2, 6, True),
    },
    "contact-velocity": {
        "impact_chirp_pitch": (0.25, 4, True),
        "impact_tone_noise": (0, 1, False),
        "impact_width": (0.25, 4, True),
        "impact_noise_tilt": (-24, 24, False),
        "direct_gain": (0, 2, False),
        "velocity_brightness": (0, 12, False),
    },
    "texture": {
        "field_turbulence": (0.15, 3, True),
        "field_turbulence_slope": (-0.5, 1, False),
        "field_packet_spread": (0.2, 5, True),
        "field_phase_bandwidth": (0.0001, 0.03, True),
        "field_phase_tilt": (-2, 2, False),
    },
    "temporal-final": {
        "body_brightness": (-24, 12, False),
        "bloom_rate": (0.005, 16, True),
        "bloom_energy_acceleration": (0, 1, False),
        "body_decay_seconds_0": (0.2, 8, True),
        "body_decay_seconds_7": (0.2, 6, True),
    },
}
