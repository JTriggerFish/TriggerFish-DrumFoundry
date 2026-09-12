# DrumFoundry development

- Use `dev.ps1` for builds/tests; Windows uses MSYS2 MinGW64 GCC and Ninja only.
  Do not introduce MSVC presets, targets, build directories or fallback selection.
- Read DEVELOPMENT.md and docs/engine.md before changing build/runtime boundaries.
- Python is optional offline testing/analysis/fitting. It calls native C++ directly.
  No Node, Wasm, browser server or Python dependency in the audio runtime.
- Keep DSP, patch validation, parameter metadata, runtime ownership, host adapters
  and UI separate. Prefer short documented functions and focused files.
- Preserve existing synthesis/parameter behaviour during migration. Compare
  deterministic and repeated-hit renders before claiming parity; never infer a
  perceptual improvement from a single numerical score.
- Never silently normalize audio or add hidden coefficients. JSON, native control
  values and eventual UI must share one authoritative mapping.
- Keep references and output artifacts outside Git. Never overwrite user fits.
- Work on `dev`. Do not remove code from TriggerFish-VCV until explicitly agreed.
