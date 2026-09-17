# Topology modules

The editor separates **audio connections** from **body-energy attachments**.
Audio routes carry signals between the recipe's exciters, mixers, resonator and
output. An attachment modifies the resonator's existing modal state: it is not
another output or an independently mixed noise source.

## Rim contact

Open **Routing**, click the diagram, then choose **Modules…** to add,
bypass or remove rim contact. It can attach to any current metallic or membrane
body: cymbal, gong, kick, tom or snare. Its controls appear together in the left
column, with the same green accent as its diagram connection. Bypass retains
the settings; removal removes the node and its controls. Both are undoable.

The existing [contact model](hi-hat-contact.md) runs after modal excitation and
before observation. Collisions exchange modal and rattle energy; impact loss
and settling dissipate energy. Moving the separation control supplies explicit
actuator work. On membranes, displacement conversion follows the tension-scaled
frequencies. Live modal edits retain both body and contact state.

Separation, clearance, damping, motion, settling and closure strength remain
live-editable. MIDI CC4 controls separation on any attached body (127 closed,
0 open). Adding/removing a module is a structural edit and uses the normal
voice replacement lifecycle; it is not promised to retain a ringing tail.

## Stored representation

The module owns its parameters once, under a separate node:

```json
{
  "id": "rim-contact",
  "type": "interaction.rim-contact",
  "version": 1,
  "parameters": {"hat_contact_enabled": 1, "hat_openness": 1}
}
```

Omitted parameters expand from native descriptors when loaded. The complete
effective values are returned/saved, just like every other module. The historical
`hat_*` keys and CLAP IDs remain stable; they no longer imply a hi-hat-only owner.
Legacy metallic patches move these values out of `body` during native loading,
without changing numbers or modifying the source file. Conflicting ownership
is rejected, not silently merged.

The patch has a separate attachment list, for example:

```json
"attachments": [
  {"module": "rim-contact", "body": "body", "port": "modal-state"}
]
```

The target is `body` for metallic recipes, `kick-resonance` for kicks and
`membrane-body` for toms/snares. A present module requires exactly one valid
attachment. Missing nodes, wrong targets and audio/state port mismatches are
errors. Attachments have no gain or extra enable coefficient: the module's
enable control in the routing editor's **Modules…** menu is the only bypass.
The regular parameter cards do not duplicate that switch. Bypassing keeps the
node, attachment and all of its settings; removing it deletes the module.

## Execution and scope

JSON is validated and prepared off the audio thread. Processing remains compiled
sample-by-sample calls with bounded storage: no graph traversal, allocations,
JIT or second voice crossfade. An absent module cannot be activated by automation;
its controls stay registered with stable host IDs but are hidden and ignored.

This is the first optional interaction module. The underlying exciter/body/mixer
audio recipes still have their existing validated paths. The editor does **not**
yet allow arbitrary exciter swaps, multiple bodies, extra wire racks or unrestricted
feedback wiring. Further modules should use explicit compatible ports and extend
the prepared execution plan, not disguise fixed recipe switches as free routing.

## Regression checks

- Legacy/canonical migration and bypass/removal preserve deterministic PCM,
  including repeated hits; the accepted hi-hat pedal path is unchanged.
- Every recipe exercises contact attachment, rejection and live/fresh parameter
  parity. The realtime allocation/deallocation watchdog also covers attached
  metallic, kick and snare voices.
- Membrane contact tests check silence, passive energy accounting, moving closure,
  tension, state-preserving modal edits and reset at 44.1/48/96 kHz.
