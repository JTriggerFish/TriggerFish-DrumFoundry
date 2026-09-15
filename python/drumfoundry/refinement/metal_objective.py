"""Metallic comparison terms from the reviewed structured-series fitting method."""

import numpy as np
from triggerfish_percussion.spectral_bloom_loss import SpectralBloomLoss
from triggerfish_percussion.modal_texture_loss import ModalTextureLoss
from triggerfish_percussion.perceptual_fit_losses import AuralossMel
from triggerfish_percussion.reference_floor_mel import ReferenceFloorMel


class Objective:
    units = "bloom/10 + 0.6 texture + 0.15 Mel"

    def __init__(self, reference, rate):
        self.shape = SpectralBloomLoss(reference, rate)
        self.texture = ModalTextureLoss(reference, rate)
        self.mel = AuralossMel(reference, rate)
        self.specification = dict(
            version="structured-metal-texture-v1",
            bloom=self.shape.specification,
            texture=self.texture.specification,
            mel=self.mel.specification,
            weights=[0.1, 0.6, 0.15],
        )

    def components(self, audio):
        return dict(
            bloom=float(np.linalg.norm(self.shape.residual(audio))),
            texture=self.texture.score(audio),
            mel=self.mel.score(audio),
        )

    def score(self, audio):
        c = self.components(audio)
        return 0.1 * c["bloom"] + 0.6 * c["texture"] + 0.15 * c["mel"]

    def residual(self, audio, regions=None):
        return np.array([np.sqrt(self.score(audio))])


class CrashObjective(Objective):
    """Spectral/attack-led proposal score; texture is not allowed to dominate."""

    units = "Mel + 0.3 attack Mel + 0.05 bloom + 0.3 texture"

    def __init__(self, reference, rate, reference_floor_db=None):
        super().__init__(reference, rate)
        self.attack_frames = round(0.3 * rate)
        self.attack = AuralossMel(reference[: self.attack_frames], rate)
        self.specification.update(
            version="crash-low-blur-v1",
            weights=[1, 0.3, 0.05, 0.3],
            order=["mel", "attack_mel", "bloom", "texture"],
            attack=self.attack.specification,
            attack_seconds=0.3,
        )
        if reference_floor_db is not None:
            self.mel = ReferenceFloorMel(reference, rate, reference_floor_db)
            self.attack = ReferenceFloorMel(
                reference[: self.attack_frames], rate, reference_floor_db
            )
            self.specification.update(
                version="crash-reference-floor-v2",
                mel=self.mel.specification,
                attack=self.attack.specification,
            )

    def components(self, audio):
        return dict(
            super().components(audio),
            attack_mel=self.attack.score(audio[: self.attack_frames]),
        )

    def score(self, audio):
        return self.score_components(self.components(audio))

    @staticmethod
    def score_components(c):
        """Keep batch/seed audits on exactly the same declared weighting."""
        return c["mel"] + 0.3 * c["attack_mel"] + 0.05 * c["bloom"] + 0.3 * c["texture"]
