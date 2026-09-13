# Native workbench migration

The Visage editor is shared by CLAP and the standalone. Python remains optional
offline analysis/fitting; no browser, Node or Wasm renderer is introduced.
The reference is TriggerFish-VCV's `workbench/web`, preserved there unchanged.

## Layout and interaction contract

- Header: presets/reference selection, playback, master, persistent limiter
  reduction/latency/bypass and standalone settings.
- Left: two independently laid-out control columns, excitation/output and
  resonance/bloom, with the T60 editor retained in its existing control area.
- Right: waveform/spectrogram, strike surface with implement controls, wide
  modal editor. Resizable analysis/editor split; controls may scroll without
  moving playback and analysis out of reach.
- Bottom: named snapshots, fit save/load and persistent errors (never swallowed).
- Preserve double-click defaults, curve editing, modal painting/harmonic tools,
  reference-only colour normalization, mirrored alignment and black-centred
  difference display. No automatic audio normalization.

Use Visage buttons, menus, text editors and scrolling directly. The library has
no stock audio slider: a small conventional horizontal Frame-based control is
needed. Custom DSP editors and graphs use Visage drawing, not browser widgets.
Do not redesign the control surface or change presets during this port.

## Iterative delivery

1. Optional pinned Visage build, shared editor foundation, native performance
   controls, standalone window and CLAP embedding/lifecycle.
2. Native settings/device selection and shared patch editing, save/load/snapshots.
3. Port recipe-aware parameter grouping and native modal/T60/meta editors.
4. Reference selection/playback and off-thread native rendering/FFT/STFT,
   GPU presentation, comparison/alignment and interactive analysis.

Review/test each chunk, commit and push to dev, and check all four CI platforms.
Incomplete panels must say so; placeholder graphics are not measurements.
Audio callbacks never touch Visage, allocate display data or wait for rendering.

## Progress

- Foundation: shared native content, stock Visage buttons/popups, conventional
  sliders with double-click reset/fine drag, strike pad, limiter/status/error
  readouts and standalone `--gui` path. `--ui-smoke` captures a real GPU frame and
  closes without opening devices. Interaction tests do not require a display.
- CLAP embedding, full settings and the DSP/analysis editors remain next.

## References

- [Visage](https://github.com/VitalAudio/visage/tree/828037000d0893647ab29b66ae9c4a241c90f671):
  MIT, GPU graphics, native windows, standard widgets and plugin embedding.
- [CLAP GUI contract](https://github.com/free-audio/clap/blob/195b42a004144fab0b3cf95e9c067187d15365b7/include/clap/ext/gui.h):
  lifecycle, parent embedding, scale and resizing.
