"""Select a declared loss profile without changing the synthesizer."""

import numpy as np


def objective_for(profile, target, baseline, rate):
    if profile == "metal":
        import torch

        torch.set_num_threads(1)
        from .metal_balance import CrashBalance

        loss = CrashBalance(target, rate)
        return loss, loss.components
    if profile == "kick":
        from triggerfish_percussion.ridge_balance_loss import RidgeBalanceLoss

        loss = RidgeBalanceLoss(target, baseline, rate)
    elif profile == "gong-onset":
        from .gong_onset import GongOnsetLoss

        loss = GongOnsetLoss(target, rate)
    elif profile == "short-drum":
        from triggerfish_percussion.short_drum_fit_loss import ShortDrumLoss

        loss = ShortDrumLoss(target, rate)
    else:
        raise ValueError(f"Unknown loss profile: {profile}")

    def components(audio):
        return dict(
            loss.diagnostics(audio), score=float(np.linalg.norm(loss.residual(audio)))
        )

    return loss, components
