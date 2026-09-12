"""Offline rendering entry point. Writes unnormalized IEEE float WAV."""

import argparse
from pathlib import Path

from .renderer import Renderer


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("preset", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--seconds", type=float, default=6)
    parser.add_argument("--sample-rate", type=int, default=48000)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Output already exists; choose a new filename")
    from scipy.io.wavfile import write

    with Renderer(args.preset, args.sample_rate) as renderer:
        audio = renderer.render(args.seconds)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    write(args.output, args.sample_rate, audio)
    print(f"Rendered {len(audio)} frames to {args.output}; no gain matching or limiter")


if __name__ == "__main__":
    main()
