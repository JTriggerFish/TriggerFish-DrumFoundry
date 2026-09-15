# Development

## Toolchains

Windows: MSYS2 MinGW64 GCC, CMake and Ninja under `C:/msys64/mingw64/bin`.
Set `MSYS2_ROOT` to relocate MSYS2. No Visual Studio preset, target or fallback.
Install MSYS2 packages `mingw-w64-x86_64-gcc`, `mingw-w64-x86_64-cmake` and
`mingw-w64-x86_64-ninja` if absent.

Linux/macOS: CMake >=3.25, Ninja and the native C++ compiler, plus PowerShell 7
to use `dev.ps1`. Alternatively use `cmake --preset native` and
`cmake --build build/native`. Windows uses `cmake --preset mingw`.

Optional development: uv plus managed Python 3.13. `uv.lock` pins packages;
`./dev.ps1 setup` creates `.venv`, installs Black/notebook hooks
and sets repository-local LF handling. It does not edit global
Git configuration or install system tools. Configure your own Git identity.

## Commands

| Command | Purpose |
|---|---|
| `./dev.ps1 doctor` | Inspect native tools and Git state |
| `./dev.ps1 build` | Compile engine and C interface, without Python |
| `./dev.ps1 test` | Native DSP tests |
| `./dev.ps1 clap` | Build the optional headless CLAP preview |
| `./dev.ps1 clap-test` | DSP tests plus dynamic plugin/host integration tests |
| `./dev.ps1 clap-dist` | Package CLAP preview under `dist/clap` |
| `./dev.ps1 standalone` | Build the optional console audio/MIDI application |
| `./dev.ps1 standalone-test` | Native tests plus hardware-free standalone checks |
| `./dev.ps1 standalone-dist` | Package standalone under `dist/standalone` |
| `./dev.ps1 ui` | Optional Visage workbench build, including standalone |
| `./dev.ps1 ui-test` | UI control tests plus the native suites |
| `./dev.ps1 ui-dist` | Package native UI preview under `dist/ui` |
| `./dev.ps1 setup` | Locked Python environment and local Git hooks |
| `./dev.ps1 python-test` | Native binding, fitting and analysis tests |
| `./dev.ps1 test-fitting-tools` | Alias for the development tests |
| `./dev.ps1 perceptual-test` | Opt-in published-loss smoke tests; install the `perceptual-fit` group first |
| `./dev.ps1 benchmark-percussion` | Optional component and realtime-block DSP benchmark |
| `./dev.ps1 check` | Run all formatting/notebook hooks |
| `./dev.ps1 dist` | Package an engine preview, not a CLAP plugin |

Use `-Jobs N` to control native build parallelism. `-Python path` is an explicit
test interpreter override. No command deletes build directories or user presets.

Git development branch: `dev`. CI builds and tests Windows/MinGW, Linux,
macOS ARM64 and macOS x64. Each native build/package precedes the optional Python
job steps, demonstrating that Python is not required to build the engine.
Engine, CLAP and standalone preview artifacts are uploaded per runner; tagged public release
automation and application bundles remain future work.

## Code boundaries

Keep files focused and new integration functions short. `engine/tfdsp` holds
the DSP components and instrument implementations.
The native runtime, patch validator, descriptor access and C ABI are separate.
Python search orchestration must never reimplement the synthesis or control map.
Keep recordings and render outputs in ignored `data/`, `renders/` or `build/`.

Third-party dependencies are version/hash pinned in `cmake/dependencies.cmake`.
Set standard `FETCHCONTENT_SOURCE_DIR_EIGEN` / `FETCHCONTENT_SOURCE_DIR_JSON`
cache variables to local copies for offline builds.
