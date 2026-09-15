# Optional reference libraries

References are audition/analysis attachments, not synthesis inputs. A fresh
session and the ordinary factory presets use **None**, showing one model
spectrogram. No reference library is required to play, edit or save a sound.

## Using the library

1. Choose **Settings → Reference library folder…**. Paste a folder path or browse,
   then choose **Use this folder**. This local preference is shared by the native
   standalone and CLAP editor; audio/MIDI settings remain separate.
2. Open **Reference: None** above the plot. None is always first. Folder entries
   open their contents, and **Parent folder** goes back up. Choose a WAV file.
3. **Hide reference** keeps the attachment but shows only the model. **Show
   reference** restores the comparison. Choosing **None** removes the attachment.
4. Save a user preset normally. It retains the relative file path,
   visibility, explicit reference gain, channel, alignment and content identity.

Local calibrations appear under **Presets → User presets** with their folder
hierarchy. They are not factory content and are never packaged or published.
Moving between samples never changes the instrument or its strike parameters.

## Storage and portability

The base folder is stored in `reference-library.json` beside the local `fits`
folder (on Windows: `%LOCALAPPDATA%/TriggerFish/DrumFoundry`). It is not included
in presets or CLAP state. Clear it through Settings without deleting any files.

A fit's optional `reference` object uses a UTF-8, forward-slash `libraryPath`
relative to that base folder. `visible`, `referenceGainDb`, `channel` and
`offsetSeconds` describe presentation only. SHA256 identifies the raw WAV file;
it is optional for a newly selected file. Null or an absent object means None.
Legacy calibration URLs are converted to relative paths on load. Existing
corpus/cell annotations are retained for offline analysis tools; the native UI
does not depend on them. Absolute sample paths are not written back to fits.

The existing private calibration folders can be used unchanged under a common
base. This migration does not move/delete recordings or overwrite user fits.
Calibration presets and reference audio both remain local, outside Git.

## Failure and rendering contract

- Missing folders/files, unsupported WAVs or a hash mismatch show a reference
  warning. The attachment remains saveable and synthesis still renders/plays.
- A missing or hidden reference uses the single-model view. Comparison controls
  return when a visible reference has loaded. Hiding is presentation-only: it
  does not re-trigger the voice or restart a render.
- Reference decoding, hashing, resampling and STFT run off the UI/audio threads.
  Preview synthesis uses the audition/device sample rate, never the file's rate.
- Colour scaling is reference-based when comparing, otherwise fixed at 0 dBFS.
  There is no automatic audio normalization. Reference gain is explicit.
- Folder browsing reads one directory at a time. Only WAVs and folders are
  offered. Paths cannot escape the configured library; symlink entries are not
  browsed. Oversized directories produce a warning rather than a silent cutoff.

Regression coverage lives in `tests/ui/library_tests.cpp`,
`tests/ui/reference_tests.cpp` and `tests/clap/editor_events.cpp`.
Migration tests fingerprint the instrument and strike defaults independently
of reference/view metadata. The fingerprints were derived from the original
byte-verified presets; the legacy PCM signatures and tolerances are unchanged.
