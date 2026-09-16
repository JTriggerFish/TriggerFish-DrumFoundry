"""Scheduling only: every layer uses the same native renderer and seed policy."""

from concurrent.futures import ThreadPoolExecutor, wait
from .saved import SavedFitRenderer


class LayerEvaluation:
    """One borrowed sequential voice, or independent owned concurrent voices.

    Calls to measure must not overlap. Worker tasks never share a native voice;
    Results preserve layer order, so scheduling cannot change the objective.
    A failed call drains its workers before returning, making retries safe.
    """

    def __init__(self, saved, layers, seconds, workers):
        if isinstance(workers, bool) or int(workers) != workers or workers < 1:
            raise ValueError("Worker count must be a positive integer")
        self.layers, self.seconds = layers, seconds
        self.voices, self.pool, self.closed = [], None, False
        self.saved = saved
        if workers > 1:
            try:
                for _ in layers:
                    self.voices.append(SavedFitRenderer(saved.fit, saved.sample_rate))
                self.pool = ThreadPoolExecutor(
                    max_workers=min(int(workers), len(layers))
                )
            except BaseException:
                self.close()
                raise

    def seeds(self, seed):
        return [layer.get("training_seed", seed) for layer in self.layers]

    def measure(self, parameters, seed, measure):
        """Run measure(loss, rendered_audio) with identical serial/parallel inputs."""
        if self.closed:
            raise RuntimeError("Layer evaluator is closed")
        seeds = self.seeds(seed)

        def one(index):
            layer = self.layers[index]
            voice = self.voices[index] if self.pool else self.saved
            audio = voice.render(
                parameters, self.seconds, seed=seeds[index], event=layer["event"]
            )
            return measure(layer["loss"], audio)

        if not self.pool:
            return list(map(one, range(len(self.layers))))
        return self._parallel(one)

    def _parallel(self, one):
        """Drain even partially submitted batches before callers can reuse voices."""
        futures = []
        try:
            for index in range(len(self.layers)):
                futures.append(self.pool.submit(one, index))
            return [future.result() for future in futures]
        except BaseException:
            for future in futures:
                future.cancel()
            wait(futures)
            raise

    def close(self):
        """Finish workers before freeing their voices; never close the borrowed voice."""
        if self.pool:
            self.pool.shutdown(wait=True)
        for voice in self.voices:
            voice.close()
        self.voices.clear()
        self.pool = None
        self.closed = True
