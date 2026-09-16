"""Scheduling and lifetime checks independent of a fitting objective."""

import pytest
from concurrent.futures import ThreadPoolExecutor
from threading import Event
from drumfoundry.refinement import layer_evaluation


class FakeVoice:
    def __init__(self, *args):
        self.fit, self.sample_rate, self.closed = {}, 48000, False

    def render(self, parameters, seconds, seed, event):
        return (parameters, seconds, seed, event)

    def close(self):
        self.closed = True


@pytest.mark.parametrize("workers", [1, 2, 8])
def test_worker_count_changes_only_scheduling(monkeypatch, workers):
    owned = []

    def create(*args):
        voice = FakeVoice()
        owned.append(voice)
        return voice

    monkeypatch.setattr(layer_evaluation, "SavedFitRenderer", create)
    saved = FakeVoice()
    layers = [dict(event={}, loss=None, training_seed=12), dict(event={}, loss=None)]
    evaluation = layer_evaluation.LayerEvaluation(saved, layers, 3, workers)
    assert evaluation.measure({}, 91, lambda loss, audio: audio) == [
        ({}, 3, 12, {}),
        ({}, 3, 91, {}),
    ]
    evaluation.close()
    evaluation.close()
    assert not saved.closed and all(v.closed for v in owned)
    with pytest.raises(RuntimeError, match="closed"):
        evaluation.measure({}, 91, lambda loss, audio: audio)


def test_partial_worker_construction_cleans_up(monkeypatch):
    voice = FakeVoice()
    calls = []

    def create(*args):
        calls.append(1)
        if len(calls) == 2:
            raise ValueError("invalid document")
        return voice

    monkeypatch.setattr(layer_evaluation, "SavedFitRenderer", create)
    with pytest.raises(ValueError, match="invalid document"):
        layer_evaluation.LayerEvaluation(FakeVoice(), [{}, {}], 3, 2)
    assert voice.closed


@pytest.mark.parametrize("workers", [0, -1, 1.5, True])
def test_invalid_worker_counts(workers):
    with pytest.raises(ValueError):
        layer_evaluation.LayerEvaluation(FakeVoice(), [{}], 3, workers)


@pytest.mark.parametrize("failure", ["render", "measure", "submit"])
def test_failed_batch_drains_before_retry(monkeypatch, failure):
    started, release, failed, returned = (Event() for _ in range(4))
    busy = []

    class Voice(FakeVoice):
        def render(self, parameters, seconds, seed, event):
            if event == 1 and parameters.get("fail"):
                busy.append(True)
                started.set()
                assert release.wait(5)
                busy.clear()
            if event == 0 and parameters.get("fail") and failure == "render":
                assert started.wait(5)
                failed.set()
                raise ValueError("render failure")
            if not parameters.get("fail"):
                assert not busy, "retry reused a voice while a failed batch ran"
            return parameters

    monkeypatch.setattr(layer_evaluation, "SavedFitRenderer", Voice)
    # Runtime failure first exposes map's early-return bug; submission failure
    # needs its blocking task first to exercise partial-batch cleanup instead.
    order = [1, 0] if failure == "submit" else [0, 1]
    layers = [dict(event=index, loss=index) for index in order]
    evaluation = layer_evaluation.LayerEvaluation(FakeVoice(), layers, 1, 2)
    submit = evaluation.pool.submit

    def submit_or_fail(one, index):
        if index == 1:
            assert started.wait(5)
            failed.set()
            raise ValueError("submit failure")
        return submit(one, index)

    def measure(loss, audio):
        if failure == "measure" and loss == 0 and audio.get("fail"):
            assert started.wait(5)
            failed.set()
            raise ValueError("measure failure")
        return loss

    def run():
        try:
            return evaluation.measure({"fail": True}, 1, measure)
        finally:
            returned.set()

    if failure == "submit":
        monkeypatch.setattr(evaluation.pool, "submit", submit_or_fail)
    try:
        with ThreadPoolExecutor(max_workers=1) as caller:
            future = caller.submit(run)
            try:
                assert failed.wait(5)
                assert not returned.wait(0.05)
            finally:
                release.set()
            with pytest.raises(ValueError, match=failure + " failure"):
                future.result(timeout=5)
        monkeypatch.setattr(evaluation.pool, "submit", submit)
        assert evaluation.measure({}, 1, measure) == order
    finally:
        release.set()
        evaluation.close()
