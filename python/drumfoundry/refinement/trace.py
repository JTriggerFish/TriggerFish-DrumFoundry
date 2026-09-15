"""Seed-separated native evaluation and a complete candidate trace."""

import numpy as np


def evaluator(saved, args, loss, components, seeds, rows):
    """Build a stage callback; record parameters and each seed's component losses."""

    def evaluate(values, stage, residual=False):
        audio = [saved.render(values, args.seconds, seed) for seed in seeds]
        metrics = [components(signal) for signal in audio]
        scores = np.array([m["score"] for m in metrics])
        score = float(
            np.mean(scores) if args.profile == "metal" else np.sqrt(np.mean(scores**2))
        )
        if not np.isfinite(score):
            raise ValueError("Non-finite fitting objective")
        rows.append(
            dict(
                stage=stage,
                parameters=dict(values),
                components=metrics,
                score=score,
            )
        )
        print(f"{len(rows)}: {stage}: {score:.6g}", flush=True)
        if residual:
            return np.concatenate(
                [loss.residual(signal) for signal in audio]
            ) / np.sqrt(len(seeds))
        return score

    return evaluate
