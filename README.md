# TriggerFish DrumFoundry

A modular percussion synthesizer for kicks, snares, toms, cymbals and gongs.
Available as a CLAP plugin and standalone application for Windows, Linux and macOS.

The native Visage editor combines live MIDI performance, graphical modal and
decay editing, output EQ, waveform/spectrogram analysis and saved presets.
Optional reference libraries support side-by-side sound design and calibration.
The output limiter has a visible bypass switch and 1 ms lookahead.

## Build and run

Requires PowerShell 7, CMake and Ninja. Windows uses MSYS2 MinGW64 GCC;
Linux uses GCC or Clang, and macOS uses Apple Clang.

```powershell
./dev.ps1 ui-test
./dev.ps1 ui-dist
./build/native/TriggerFishDrumFoundry.exe
```

Omit `.exe` on Linux/macOS. Choose your audio/MIDI devices in **Settings**;
saved device settings are restored on launch. Windows supports ASIO and WASAPI.
The build also produces the CLAP plugin with the same editor.
Packages are development previews, not signed releases.

## Optional development tools

Python provides offline rendering, analysis and fitting through the native C++
engine. It is not required to build or run the plugin or standalone.

```powershell
./dev.ps1 setup
./dev.ps1 python-test
```

See [development setup](DEVELOPMENT.md), [audio/MIDI settings](docs/standalone.md),
[reference libraries](docs/reference-libraries.md) and [fitting](docs/fitting.md).

## Licence

[GNU GPLv3](LICENSE.txt). Third-party components retain their respective licences.
