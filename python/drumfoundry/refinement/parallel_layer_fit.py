"""Independent native voices evaluate a fixed velocity grid concurrently."""

from concurrent.futures import ThreadPoolExecutor
import numpy as np
from .layer_fit import LayerFit
from .saved import SavedFitRenderer


class ParallelLayerFit(LayerFit):
    """Same objective and events as LayerFit, never concurrent access to a voice."""

    def __init__(self, saved, layers, parameters, seconds=3):
        super().__init__(saved, layers, parameters, seconds)
        self.voices = [SavedFitRenderer(saved.fit, saved.sample_rate) for _ in layers]
        self.pool = ThreadPoolExecutor(max_workers=len(layers))

    def close(self):
        self.pool.shutdown(wait=True)
        for voice in self.voices:
            voice.close()

    def evaluate(self, parameters, label, seed=1944, accept=True):
        def one(index):
            layer = self.layers[index]
            audio = self.voices[index].render(
                parameters,
                self.seconds,
                seed=layer.get("training_seed", seed),
                event=layer["event"],
            )
            return layer["loss"].diagnostics(audio)

        parts = list(self.pool.map(one, range(len(self.layers))))
        errors = [part["error_db"] for part in parts]
        score = float(np.sqrt(np.mean(np.square(errors))))
        if not np.isfinite(score):
            raise ValueError("Non-finite layer loss")
        self.rows.append(
            dict(
                stage=label,
                seed=seed,
                layer_seeds=[layer.get("training_seed", seed) for layer in self.layers],
                score=score,
                layer_errors_db=errors,
                components=parts,
                parameters=dict(parameters),
            )
        )
        if accept and score < self.score:
            self.score, self.best = score, dict(parameters)
        if len(self.rows) % 20 == 0:
            print(
                label, "trial", len(self.rows), "best", round(self.score, 4), flush=True
            )
        return score
