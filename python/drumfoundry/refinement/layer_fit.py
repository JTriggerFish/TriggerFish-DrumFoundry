"""Joint-velocity native fitting with explicit bounds and complete trial logging."""

import numpy as np
from scipy.optimize import minimize
from drumfoundry.fitting import _coordinates
from .layer_evaluation import LayerEvaluation


class LayerFit:
    """One parameter vector serves every layer; no per-hit fitted gain/velocity."""

    def __init__(self, saved, layers, parameters, seconds=3, *, workers=1):
        if not layers:
            raise ValueError("At least one reference layer is required")
        self.saved, self.layers, self.seconds = saved, layers, seconds
        self.best = dict(parameters)
        self.score = float("inf")
        self.rows = []
        self.evaluation = LayerEvaluation(saved, layers, seconds, workers)

    def close(self):
        self.evaluation.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def evaluate(self, parameters, label, seed=1944, accept=True):
        parts = self.evaluation.measure(
            parameters, seed, lambda loss, audio: loss.diagnostics(audio)
        )
        errors = [part["error_db"] for part in parts]
        score = float(np.sqrt(np.mean(np.square(errors))))
        return self.record(
            parameters,
            label,
            score,
            seed,
            accept,
            components=parts,
            layer_errors_db=errors,
        )

    def record(self, parameters, label, score, seed=1944, accept=True, **details):
        """One trial/acceptance contract for scalar and residual-based searches."""
        if not np.isfinite(score):
            raise ValueError("Layer objective returned a non-finite score")
        self.rows.append(
            dict(
                stage=label,
                seed=seed,
                layer_seeds=self.evaluation.seeds(seed),
                score=score,
                parameters=dict(parameters),
                **details,
            )
        )
        if accept and score < self.score:
            self.score, self.best = score, dict(parameters)
        return score

    def stage(self, bounds, budget, label):
        """Powell in normalized coordinates; logarithmic domains are explicit."""
        if (
            not bounds
            or isinstance(budget, bool)
            or int(budget) != budget
            or budget < 1
        ):
            raise ValueError("Provide bounds and a positive evaluation budget")
        base = dict(self.best)
        names = list(bounds)
        domains = np.array([bounds[k][:2] for k in names], dtype=float)
        logs = np.array([bounds[k][2] for k in names], dtype=bool)
        _coordinates(
            self.saved.renderer,
            self.saved.snapshot(base),
            {k: bounds[k][:2] for k in names},
        )
        if np.any(domains[logs] <= 0):
            raise ValueError("Logarithmic bounds must be positive")
        domains[logs] = np.log(domains[logs])
        start = np.array([base[k] for k in names], dtype=float)
        start[logs] = np.log(start[logs])
        widths = domains[:, 1] - domains[:, 0]
        x = np.clip((start - domains[:, 0]) / widths, 0, 1)

        def objective(x):
            values = domains[:, 0] + x * widths
            values[logs] = np.exp(values[logs])
            return self.evaluate(base | dict(zip(names, values)), label)

        self.evaluate(base, label + "/start")
        minimize(
            objective,
            x,
            method="Powell",
            bounds=[(0, 1)] * len(x),
            options=dict(maxfev=budget, xtol=0.015, ftol=0.002),
        )
        return self.score
