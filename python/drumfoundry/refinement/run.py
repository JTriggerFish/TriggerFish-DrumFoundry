"""Run one explicit fitting stage; save candidates, never publish or overwrite fits."""

import json
from hashlib import sha256

from drumfoundry._native import library_path
from .contracts import prepare_output, training_seeds, audit_seeds
from .objective import objective_for
from .trace import evaluator
from .artifacts import export
from .saved import SavedFitRenderer
from .reference import load_reference, reference_attachment
from .configuration import validated


def run(args):
    """Seed-averaged selection with an unchanged baseline and held-out results."""
    args = validated(args)
    fit_bytes = args.fit.read_bytes()
    with SavedFitRenderer(json.loads(fit_bytes), args.rate) as saved:
        ref = load_reference(
            saved.fit,
            args.reference,
            args.rate,
            gain_db=args.gain,
            onset=args.onset,
            channel=args.channel,
        )
        target = ref.window(args.seconds)
        attachment = reference_attachment(
            saved.fit, ref, args.reference, getattr(args, "library_root", None)
        )
        baseline = saved.render(saved.initial, args.seconds)
        loss, components = objective_for(args.profile, target, baseline, args.rate)
        seeds = training_seeds(saved.fit["controls"]["event"]["seed"])
        holdout = audit_seeds(seeds[0])
        prepare_output(args.output)
        rows = []

        evaluate = evaluator(saved, args, loss, components, seeds, rows)
        evaluate(saved.initial, "unchanged baseline")
        from .stages import search

        search_report = search(args, saved, rows, evaluate)
        best = min(rows, key=lambda row: row["score"])
        candidate = saved.snapshot(best["parameters"], args.name)
        # Export explicit reference conditioning with the candidate.
        candidate["reference"] = attachment
        report = dict(
            profile=args.profile,
            stage=args.stage,
            budget=args.budget,
            solver=getattr(args, "solver", "powell"),
            solver_report=search_report,
            seed_aggregation=(
                "mean scalar" if args.profile == "metal" else "RMS residual norm"
            ),
            bounds=args.coordinate_bounds,
            duration_seconds=args.seconds,
            source_sha256=sha256(fit_bytes).hexdigest(),
            reference=ref.provenance,
            renderer_sha256=sha256(library_path().read_bytes()).hexdigest(),
            objective=loss.specification,
            training_seeds=seeds,
            audit_seeds=holdout,
            rows=rows,
            selected=best,
            acceptance="candidate only; inspect and audition",
            holdout=[
                components(saved.render(best["parameters"], args.seconds, seed))
                for seed in holdout
            ],
        )
        export(args, saved, candidate, report, rows, target, baseline, best)
        return candidate, report
