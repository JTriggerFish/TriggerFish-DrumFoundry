"""Optional fixed-level Plotly diagnostics; never a second audition surface."""

import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from scipy.signal import stft, welch


def plots(signals, rate, output, *, title="Crash", seconds=6):
    """Every plot uses fixed gains and identical analysis on source and synth."""
    bands = [
        (100, 300),
        (300, 700),
        (700, 1500),
        (1500, 3000),
        (3000, 6000),
        (6000, 16000),
    ]
    fig = make_subplots(
        rows=4,
        cols=2,
        subplot_titles=[
            "40–300 ms spectrum",
            "300–1000 ms spectrum",
            *[f"{lo}–{hi} Hz envelope" for lo, hi in bands],
        ],
    )
    for (name, audio), colour in zip(
        signals.items(), ["#eebc59", "#8d91a4", "#6bb9e9"]
    ):
        for col, (a, b) in enumerate(((0.04, 0.3), (0.3, 1)), 1):
            segment = audio[round(a * rate) : round(b * rate)]
            f, p = welch(segment, rate, nperseg=min(8192, len(segment)), nfft=16384)
            keep = (f >= 60) & (f <= 16000)
            fig.add_trace(
                go.Scatter(
                    x=f[keep],
                    y=10 * np.log10(np.maximum(p[keep], 1e-20)),
                    name=name,
                    legendgroup=name,
                    line_color=colour,
                    showlegend=col == 1,
                ),
                row=1,
                col=col,
            )
        f, t, z = stft(audio, rate, nperseg=4096, noverlap=3584)
        # A cropped render's zero-padded final window is not an audible burst.
        complete = t <= (len(audio) - 2048) / rate
        for i, (lo, hi) in enumerate(bands):
            power = np.sum(abs(z[(f >= lo) & (f < hi)]) ** 2, axis=0)
            fig.add_trace(
                go.Scatter(
                    x=t[complete],
                    y=10 * np.log10(np.maximum(power[complete], 1e-20)),
                    name=name,
                    legendgroup=name,
                    line_color=colour,
                    showlegend=False,
                ),
                row=2 + i // 2,
                col=1 + i % 2,
            )
    fig.update_xaxes(type="log", range=[np.log10(60), np.log10(16000)], row=1)
    fig.update_yaxes(range=[-100, -20], row=1)
    for row in range(2, 5):
        fig.update_xaxes(range=[0, seconds], row=row)
        fig.update_yaxes(range=[-95, -10], row=row)
    fig.update_layout(
        template="plotly_dark",
        width=1450,
        height=1100,
        title=f"{title}: fixed-level spectrum and decay; no gain matching",
    )
    (output / "inspection.plotly.json").write_text(fig.to_json(), encoding="utf8")


def spectrograms(reference, audio, rate, output, *, seconds=4):
    """Reference-anchored colour and signed excess/missing energy, zero black."""
    spectra = [stft(x, rate, nperseg=4096, noverlap=3584) for x in (reference, audio)]
    f, t, _ = spectra[0]
    keep = (f >= 60) & (f < 16000)
    db = [20 * np.log10(np.maximum(abs(s[2][keep]), 1e-10)) for s in spectra]
    ceiling = float(db[0].max())
    floor = ceiling - 70
    fig = make_subplots(
        rows=3,
        cols=1,
        subplot_titles=[
            "Reference",
            "Candidate",
            "Difference: amber excess / blue missing",
        ],
    )
    for i, z in enumerate(db):
        fig.add_trace(
            go.Heatmap(
                x=t,
                y=np.log10(f[keep]),
                z=z,
                zmin=floor,
                zmax=ceiling,
                colorscale="Magma",
                showscale=False,
            ),
            row=i + 1,
            col=1,
        )
    diff = np.maximum(db[1], floor) - np.maximum(db[0], floor)
    fig.add_trace(
        go.Heatmap(
            x=t,
            y=np.log10(f[keep]),
            z=diff,
            zmin=-18,
            zmax=18,
            colorscale=[[0, "#218ad1"], [0.5, "#000000"], [1, "#efa740"]],
            showscale=False,
        ),
        row=3,
        col=1,
    )
    ticks = [100, 300, 1000, 3000, 10000]
    fig.update_yaxes(tickvals=np.log10(ticks), ticktext=[str(f) for f in ticks])
    fig.update_xaxes(range=[0, seconds])
    fig.update_layout(template="plotly_dark", width=1450, height=1100)
    (output / "spectra.plotly.json").write_text(fig.to_json(), encoding="utf8")
