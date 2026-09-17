# DrumFoundry user manual

[Overview](../README.md) · [Playing](#playing) · [Sound design](#sound-design) ·
[Modes](#editing-modes) · [References](#reference-samples) · [Presets](#saving-presets)

## First session

In the standalone, open **Settings → Audio / MIDI settings…**, choose your output
device and MIDI input, then click **Apply & start**. These settings are remembered.
Start with a 256-sample buffer; try a smaller buffer for quicker response, or a
larger one if playback crackles. Some ASIO drivers require other audio applications
to release the device first.

In a DAW, load the CLAP plugin on an instrument track and enable MIDI monitoring.
The DAW manages audio devices and buffer size.

Choose a sound from **Presets → Factory**. Keep **Limiter ON** and begin with a
low **Master** level. The bottom bar shows limiter reduction, latency and errors.
The limiter adds 1 ms while enabled. Frequent reduction can change the attack;
lower Output level to hear the sound with less limiting.

## Playing

Click the **Strike** pad: higher hits are stronger. The horizontal axis shows
the current instrument's control: bell–bow–edge for cymbals, beater hardness for
the kick. **Freeze strike…** opens fixed velocity and location controls (beater
hardness for kicks). Turn **Freeze ON** to make every pad click use those values;
the button stays highlighted. Turn it off to follow your click position again.
MIDI remains expressive. Freeze is an audition setting for the current editor.

Choose **Stick**, **Brush** or **Mallet**, then adjust hardness or firmness.
For metallic instruments, **Brush spread** lengthens the bristle gesture; it is
inactive with sticks and mallets. For drums, **Contact spread** broadens the noisy
attack for any implement. Repeated hits build on the sound already ringing.
MIDI velocity controls strike strength; MIDI notes currently
trigger the selected instrument at its set tuning.

Beside the strike pad, move **Separation / pedal** left to close or right to open.
**MIDI CC4: 127 = closed, 0 = open.** Fast closing creates a chick; slow closing
is quieter. Pedal movement changes an ongoing ring. **Hand mute** damps ringing.

## Working the interface

Scroll the two control columns independently. Drag the dividers to resize the
control area, spectrogram and modal editor. **Settings → Layout…** offers three
text sizes and switches to hide the spectrogram or modal editor on small displays.
**Settings → Colour scheme** offers Classic, LazyVim and custom JSON themes.

- Hover controls for help. Double-click a slider to reset it.
- Shift-drag sliders for fine adjustment; right-click to enter a value.
- **Undo / Redo**: Ctrl+Z and Ctrl+Y or Ctrl+Shift+Z on Windows/Linux;
  Cmd+Z and Cmd+Shift+Z on macOS.
- In a DAW, click inside the editor before using shortcuts. Use the buttons if
  the host captures them. Text fields keep their own text-editing undo.

Most controls update while the instrument rings. Judge attack changes with a
fresh hit. The spectrogram refreshes as you edit; live playing captures repeated
strikes and their accumulated sound.

## Sound design

Start from a nearby preset and save a copy. Shape the attack, set the body's
tuning, then adjust bloom and decay. Add movement gradually and finish with EQ.
Check soft, hard and repeated hits as you go.

### Attack and resonance

| Control | Effect |
|---|---|
| Contact presence / Contact level | Brings the initial contact forward against the ring. |
| Impact: ping to noise | Balances pitched ping and noisy impact. |
| Contact width | Short for a sharp tap; longer for a softer, broader impact. |
| Ping pitch / Impact noise tilt | Tunes and brightens the contact. |
| Strike accent | Adds a short hiss over the attack; Amount, Decay and Colour shape it. |
| Velocity brightness | Makes stronger hits brighter. |
| Body excitation | Drives the ringing body and bloom harder. |
| Body observation level | Adjusts the audible body balance. |
| Body tune | Moves ringing pitches together. |

**Size meta** moves several controls to give a broad starting point.
**Bloom timing…** adjusts a group of settings towards an earlier or later bloom.
Check the resulting settings and use Undo to compare.

### Bloom and decay

**Initial excitation tilt** and **Excitation centre** shape the starting brightness.
A darker start leaves more room for treble to develop. **Diffusion strength** sets
how quickly ringing spreads through the spectrum. Lower **Concentration dependence**
can let the upper bloom develop more readily. **Energy sensitivity** makes strong
and repeated hits spread faster.

The **Modal T60** curve controls decay across frequency. T60 is the time for an
isolated ring to fall by 60 dB. Bloom and contact also affect its audible duration.

- Drag a point up for longer ringing, down for shorter ringing.
- Drag the centre diamond to lengthen or shorten the whole curve.
- Double-click empty space to add a point, up to eight total.
- Double-click an interior point, or select it and use **Delete knot**, to remove
  it. The two boundary points remain.
- Use Shift-drag or the selected point's numeric controls for precision.

Start with a simple curve. Lower the treble end to shorten high ringing while
keeping the low body. **Hold decay** attempts to preserve the decay profile as
other controls change; check the result by ear. **Tail damping** gives quiet
resonances a firmer ending.

### Metallic texture

A modal handle sets a central ring. Its surrounding tones form a *packet*.

| Group | Controls and effect |
|---|---|
| Packet texture | **Surrounding rings at 1 kHz** sets their amount. **Bass / treble balance** favours low or high packets. **Spread** widens clusters; **Density** fills them with more tones. |
| Ring character | Chooses the spacing pattern. **Beating doublets** gives pairs with audible pulsation. |
| Beating | Speed sets the pulsation rate; depth sets its strength. Treble scaling lets higher rings beat faster. |
| Slow detuning | Gentle pitch wandering makes sustained ringing less regular. |
| Shimmer | Faster irregular movement adds flutter or sizzle. Sharing moves neighbouring tones more or less together. |
| Phase blur | Softens clear rings into a noisier wash. Its balance control favours bass or treble. |

For clear lows and sizzling highs, favour the treble with the packet balance
control, then add a little shimmer. Reduce phase blur if it becomes too hissy.
For overly obvious pulsing, lower beating depth first.

### Hi-hat and rim contact

| Control | Effect |
|---|---|
| Separation / pedal | Closed to open; responds to CC4. |
| Open clearance | Smaller gaps encourage contact and sizzle; larger gaps allow freer ringing. |
| Contact damping | Makes collisions drier and shortens the rattle. |
| Pedal strength | Sets the closing chick's strength. |
| Rattle motion | Adds variation to the contact and sizzle. |
| Settling | Left gives slower returns; right tightens the ending. Zero is the slowest setting. |

Tune the open sound first, then close the pedal gradually. Adjust damping and
clearance while testing the middle range. Try pedal-only closing too.

### Kick and snare

For **kick**, Contact shapes the beater click and noise. **Thump pitch** sets the
low note; **Pitch drop** and **Pitch fall time** shape its downward attack.
**Thump hold**, **Decay** and **Decay shape** control fullness and tail length.
**Resonance prominence** adds ringing body. **Energy pitch lift** and
**Tension recovery** give stronger hits a temporary pitch rise.

For **snare**, balance membrane ring with **Wire level**. **Sensitivity** and
**Threshold** determine how readily the wires respond. **Engagement** and
**Contact release** shape the response; **Wire decay** sets its length. Low/high
edges and brightness shape the frequency range. Density fills out the texture;
noise and modes balance hiss against metallic ring. **Persistent ring** adds a
tunable, longer-lived tone.

## Editing modes

Horizontal position is frequency; height is prominence. **Generate modes** makes
a regular starting pattern. Choose **Harmonic** for a pitched series or **Membrane**
for a drum-like pattern, set the base note and mode count, then **Replace modes**.
**Upper-mode stretch** spreads high pitches while **Protected low modes** keeps
the lower ones closer to the original pattern. **Falloff** and **Top level** set
the overall balance. Undo restores the previous set.

In **Select & move**, drag handles to change pitch and level. Drag a rectangle
around several modes, then drag inside the selection to move them together.
Double-click empty space to add a mode; double-click its circular handle to delete
it. **Delete / Backspace** removes the selection. **Ctrl-scroll** (Cmd-scroll on
macOS) adjusts selected metallic modes' local packet widths.

**Prominence brush** paints existing levels; **Paint modes** adds and shapes modes
as you drag. **Clear** removes active modes. The selected mode's controls allow
precise edits. On metallic sounds, local noisiness and sideband allocation tailor
each packet; more allocation gives it a greater share of the surrounding tones.

**Harmonic guide** displays a series at Guide pitch. Enable **Snap** to align
subsequent edits with it.

## Output EQ

Drag the left/right handles for high-pass/low-pass cutoffs. Drag the middle handle
horizontally for frequency and vertically for boost or cut. Scroll over it to
change **Q**; higher Q makes a narrower peak. Double-click a handle to reset it,
or click a readout below the graph for exact entry.

The background spectrum shows live output. Compare with **EQ on/off**, balance
the preset with **Output level**, and set listening volume with **Master**.

## Reference samples

Choose **Settings → Reference library folder…**, then browse the **Reference**
menu above the spectrogram. **None** shows the synth alone. Use the play symbol
beside the reference to listen; adjust Reference gain for a useful comparison.
Choose the **Model** view to hide a reference while keeping it selected.

**Mirror** puts both attacks at the centre: reference time runs towards the left,
synth time towards the right. Side-by-side and stacked views use conventional
time directions. **Difference** highlights mismatches: cyan means less synth
energy, amber means more, and black means little difference.

- Drag or scroll to move through time. Ctrl/Cmd-scroll zooms time; Alt-scroll zooms frequency.
- Shift-drag either sound to adjust alignment. Drag the divider to resize the views.
- **Reset zoom** restores the view. Larger FFT sizes reveal frequency detail;
  smaller sizes reveal attack timing. **Colour range** reveals quieter detail.

## Saving presets

Use **Presets → Save preset as…** for a named version; previous saves are kept.
Factory and user presets have separate menu sections. Choose your user folder
in **Settings → User preset folder…**; its subfolders appear as submenus.
The Presets menu also offers import and export.

User presets save reference selection and visibility. Audio stays in the library
folder, so keep it available when moving presets. A missing reference produces a
warning and leaves the synth playable.

## Routing

Expand **Routing**, then click the diagram for the larger editor. Switch the
available audio paths below it; drag boxes to rearrange the view. **Modules…**
adds, bypasses or removes rim contact on compatible instruments. Green connections
identify its interaction with the body. Use **Modules… → Rim contact enabled**
to switch it off without losing its settings. The selected
instrument determines the other available routes.
