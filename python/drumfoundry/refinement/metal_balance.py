"""Reference-fixed spectral, rise, texture and decay ranking; not listening approval."""

import numpy as np
from .metal_objective import CrashObjective
from triggerfish_percussion.band_decay_shape_loss import BandDecayShapeLoss


class CrashBalance:
    """Declared engineering ranking, never a substitute for listening approval."""

    def __init__(self, reference, rate):
        self.original = CrashObjective(reference, rate, 60)
        self.decay = BandDecayShapeLoss(reference, rate)
        shape = self.original.shape
        terminal = shape.target[:, -3:]
        plateau = (np.ptp(terminal, axis=1) < 1.5) & (
            shape.target.max(axis=1) - terminal.mean(axis=1) > 35
        )
        # Only quiet, flat terminal bands get a raised comparison floor.
        # Clearly decaying low rings are not treated as recording noise.
        self.tail_floor = np.where(plateau, terminal.mean(axis=1) + 10, -300)
        self.late = np.array([a >= 1.7 for a, b in shape.regions])
        self.specification = dict(
            spectral=self.original.specification,
            decay=self.decay.specification,
            weights=dict(
                mel=1, attack_mel=0.3, bloom=0.05, texture=0.15, shape_error_db=0.15
            ),
            acceptance="Inspect separate metrics, pitch, plots and held-out seeds; audition still required",
            version="crash-tail-floor-v3",
            terminal_plateau=dict(
                regions_seconds=[3, 6],
                max_range_db=1.5,
                below_peak_db=35,
                margin_db=10,
                apply_after_seconds=1.7,
                flagged_bands=np.flatnonzero(plateau).tolist(),
            ),
        )

    def components(self, audio):
        result = self.original.components(audio)
        result["bloom_unmasked"] = result["bloom"]
        shape = self.original.shape
        target, actual = shape.target.copy(), shape.db(shape.power(audio))
        target[:, self.late] = np.maximum(
            target[:, self.late], self.tail_floor[:, None]
        )
        actual[:, self.late] = np.maximum(
            actual[:, self.late], self.tail_floor[:, None]
        )
        error = (actual - target)[shape.active]
        rise = error[:, 2:9] - error[:, :1]
        result["bloom"] = float(np.sqrt(np.mean(error**2) + np.mean(rise**2)))
        result.update(self.decay.diagnostics(audio))
        result["score"] = (
            result["mel"]
            + 0.3 * result["attack_mel"]
            + 0.05 * result["bloom"]
            + 0.15 * result["texture"]
            + 0.15 * result["shape_error_db"]
        )
        return result
