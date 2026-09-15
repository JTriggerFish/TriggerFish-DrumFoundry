"""Full-document native rendering for fitting, including routing and saved hits."""

from copy import deepcopy
from datetime import datetime, timezone
from uuid import uuid4

from drumfoundry import Renderer
from drumfoundry.fitting import parameters, with_parameters


class SavedFitRenderer:
    """Own one voice; each evaluation resets it, sequences preserve restrike energy."""

    def __init__(self, fit, sample_rate=48000):
        self.renderer = Renderer(fit, sample_rate)
        self.fit = self.renderer.document
        self.sample_rate = int(self.renderer.sample_rate)
        self.initial = self.parameters(self.fit)

    @staticmethod
    def parameters(fit):
        keys = [k for n in fit["instrument"]["nodes"] for k in n["parameters"]]
        if len(keys) != len(set(keys)):
            raise ValueError("Duplicate parameter ownership")
        return parameters(fit)

    def snapshot(self, values, name=None):
        if set(values) != set(self.initial):
            raise ValueError("Parameter surface changed")
        fit = with_parameters(self.fit, values)
        if name is not None:
            fit.update(
                id=str(uuid4()),
                parentId=self.fit.get("id", ""),
                name=name,
                createdAt=datetime.now(timezone.utc).isoformat(),
            )
        return fit

    def render(self, values, seconds, seed=None, event=None):
        hit = dict(time=0, **(event or {}))
        if seed is not None:
            hit["seed"] = seed
        return self.sequence(values, seconds, [hit])

    def sequence(self, values, seconds, hits):
        self.renderer.configure(self.snapshot(values))
        return self.renderer.render(seconds, deepcopy(hits))

    def close(self):
        self.renderer.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
