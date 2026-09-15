# Colour schemes

**Settings → Colour scheme** offers **Classic (default)**, the original
workbench's charcoal, gold and blue palette, and the navy **LazyVim** alternative.
**Load JSON…** imports a custom palette immediately. Copy and edit
[themes/classic.json](../themes/classic.json) or
[themes/lazyvim.json](../themes/lazyvim.json) to create one.
The chosen palette is saved as a local preference and restored on the next
launch, in both standalone and CLAP. Other already-open editors keep their
own palette. **Classic (default)** restores the built-in default; existing saved
custom palettes are not overwritten by an application update.

The schema is `triggerfish.drumfoundry.theme/v1`: a `name` and a complete
`colors` object with the same keys as the example. Colours are `#RRGGBB`
or `#RRGGBBAA` (alpha last). Missing/unknown keys and invalid colours are
rejected without changing the current palette. Import never changes the
source file. Palettes are independent of presets and audio settings.

Background/Panel/Raised describe surfaces; Text/Muted/Heading their labels.
Both built-ins use bright light-grey secondary labels, with at least 7:1
contrast on the main panel surfaces. Values and headings remain slightly brighter.
Classic uses light-grey text on charcoal surfaces, blue curves and gold selections.
Parameter groups retain rounded cards and header bands; small blue, green or gold
markers indicate resonance, energy transfer or excitation without colouring
every label. EQ handles retain their distinct gold, pink and blue colours.
Button/ButtonHover/Selected/SelectedText also style native menus and buttons.
Strike and AxisVelocity/AxisPosition style the playing pad. Plot, Grid,
Spectrum and the Eq colours style visualisation chrome and curves.
The spectrogram's calibrated magnitude map and black-centred difference map
stay fixed: changing the interface theme must not change their meaning.

The optional LazyVim palette is adapted from Shubh Sharma's **LazyVim VS Code theme**, with
its indigo surfaces and blue accents. Its MIT notice is in
[themes/LICENSE.lazyvim.txt](../themes/LICENSE.lazyvim.txt).
