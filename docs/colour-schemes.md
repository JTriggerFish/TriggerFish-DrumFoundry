# Colour schemes

**Settings → Colour scheme → Load JSON…** applies a palette immediately.
Copy and edit [themes/lazyvim.json](../themes/lazyvim.json) to create one.
The chosen palette is saved as a local preference and restored on the next
launch, in both standalone and CLAP. Other already-open editors keep their
own palette. **Reset to LazyVim** restores the built-in default.

The schema is `triggerfish.drumfoundry.theme/v1`: a `name` and a complete
`colors` object with the same keys as the example. Colours are `#RRGGBB`
or `#RRGGBBAA` (alpha last). Missing/unknown keys and invalid colours are
rejected without changing the current palette. Import never changes the
source file. Palettes are independent of presets and audio settings.

Background/Panel/Raised describe surfaces; Text/Muted/Heading their labels.
The default uses neutral light-grey text on graduated navy surfaces. Parameter
groups have rounded cards and lighter header bands; small blue, green or yellow
markers indicate resonance, energy transfer or excitation without colouring
every label. Coral is reserved for errors and distinct plot handles.
Button/ButtonHover/Selected/SelectedText also style native menus and buttons.
Strike and AxisVelocity/AxisPosition style the playing pad. Plot, Grid,
Spectrum and the Eq colours style visualisation chrome and curves.
The spectrogram's calibrated magnitude map and black-centred difference map
stay fixed: changing the interface theme must not change their meaning.

The default is adapted from Shubh Sharma's **LazyVim VS Code theme**, with
its indigo surfaces and blue accents. Its MIT notice is in
[themes/LICENSE.lazyvim.txt](../themes/LICENSE.lazyvim.txt).
