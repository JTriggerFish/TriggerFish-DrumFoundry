# TriggerFish DrumFoundry

A constructive modular percussion synthesizer: cymbals, gongs, kicks, membranes
and snares built from reusable native DSP components.

This repository currently contains the **native engine and offline development
tools**. CLAP, the standalone application and the Visage editor are the next stage;
they are not implemented or advertised as available binaries yet.

## Build

```powershell
./dev.ps1 build
./dev.ps1 test
```

Windows uses the same MSYS2 **MinGW64 GCC** and Ninja toolchain as TriggerFish-VCV.
Linux uses GCC/Clang, macOS Apple Clang. PowerShell 7 drives the same commands on
all three. Normal native builds need neither Python nor Node/Wasm/Rack/Visage.
Pinned, hash-checked Eigen and JSON headers are downloaded on first configure.

## Optional Python rendering and fitting

```powershell
./dev.ps1 setup
./dev.ps1 python-test
uv run python -m drumfoundry presets/crash_calibration.fit.json renders/crash.wav
```

Python calls the native C++ library directly and receives float32 NumPy buffers.
There is no Node process, browser, sample server or second DSP implementation.
Output WAVs are unnormalized floating point: use sensible playback gain.

```python
from pathlib import Path
from drumfoundry import Renderer

with Renderer(Path("presets/gong_calibration.fit.json"), 48000) as voice:
    one_hit = voice.render(6)
    repeated = voice.render(6, [{"time": t} for t in (0, .5, 1, 1.5)])
```

The six imported JSON fits retain their values and reference identity. Private
reference recordings are not distributed. Do not mistake preservation of an
existing fit for a claim of perceptual accuracy.

See [development setup](DEVELOPMENT.md), [engine boundary](docs/engine.md),
[fitting](docs/fitting.md) and [migration status](docs/migration.md).

## Origin and licence

Extracted from [TriggerFish-VCV](https://github.com/JTriggerFish/TriggerFish-VCV)
commit `9f0d2e2ce0be3d51ff5e4ff45b0da5c2d7aaa621`.
Existing attribution and GPLv3 licence are retained. See [LICENSE.txt](LICENSE.txt)
and the [import manifest](docs/import-manifest.json). Eigen 5.0.1 uses MPL2;
nlohmann/json 3.12.0 uses MIT; their source notices remain in the fetched trees.
