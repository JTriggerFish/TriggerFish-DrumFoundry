# Live modal editing

Editing a sounding resonator should not retrigger it. The workbench therefore
uses two update paths, both independent of offline spectrogram rendering:

| Edit | Delivery | Sounding state |
|---|---|---|
| EQ, gains, damping, bloom rate | Existing sample-timed scalar queue | Retained |
| Modal pitch, prominence, widths, allocation, texture and movement | Prepared off-thread, adopted at next callback | Remapped and retained |
| Membrane pitch/character and kick resonance handles | Same prepared path | Fixed-slot quadrature and tension retained |
| Routing or observation delay | Existing replacement lifecycle | Reprepared |

Prepared modal edits are currently editor controls, **not CLAP automation
parameters**. Exposing them to hosts needs a coalesced preparation worker; they
must not be added to the sample-timed scalar setter merely because they can now
be edited during playback.

## One sounding instrument

`runtime/modal_edit` validates and designs the destination coefficients outside
the audio callback. `PreparedMailbox` has three ownership slots: one being read,
one pending, and one available for publication. Intermediate pending drags are
replaced by their newest complete target. Allocation and destruction happen on
the producer, never in the callback. Preset/rate replacement cancels pending
updates; ordinary scalar automation cannot be rolled back by an older geometry
snapshot.

The callback adopts the prepared coefficients and existing oscillator states.
It does not render a second voice, run another contact generator or crossfade
two complete sounds. Existing contact, EQ, limiter, mute and envelope histories
are untouched. The ongoing spectrum changes; it does not restart at time zero.

## Identity and energy

A metallic mode carries its painted-handle slot and member number through the
frequency sorts. Satellites have nested identities independent of their count.
Crossing another handle or widening a packet therefore cannot associate its
stored phase with an unrelated oscillator. Drift trajectories and shared
shimmer trajectories follow those identities rather than restarting.

Frequency edits preserve complex quadrature state. Coefficients change at the
callback boundary; this is phase-continuous retuning, not an added pitch glide.
Unit-length stochastic rotations are prepared directly, never interpolated
across the interior of the unit circle (which would introduce extra damping).

When allocation changes, each surviving packet retains its total squared
quadrature norm. Newly allocated members receive a share proportional to their
prepared squared drive weights; the surviving members retain their relative
phases and share the remaining energy. This is redistribution of existing
energy, not a new hit or loudness normalization. Observation gains ramp for
5 ms to reduce level steps; this is smoothing, not audio latency. A newly painted
packet has no inherited energy until a strike or transport excites it. Removing
a complete packet intentionally removes its energy.

Removed members also retain a **5 ms observation-only fade** at their nominal
frequencies, with their current phase, gain and damping. This prevents a sample
step when their contribution disappears. The temporary samples receive no
strikes, modulation or coupling and never feed energy back into the body;
they are not counted again as stored body energy. Mute still damps them and
Reset/panic clears them immediately. There is no second full voice or limiter
delay. Only edits that remove sounding members need this bounded extra work.

While that fade is active, the latest prepared modal edit stays in the mailbox;
intermediate drag positions are coalesced, not accumulated. It is adopted at the
first callback after the fade completes (at most 5 ms plus callback scheduling).
Scalar automation and strikes continue normally. Direct runtime callers check
`CanApplyModalEdit()` / the boolean adoption result and retry their latest target.

Excitation tilt, centre and launch weights describe future input, not a way to
rewrite the history of an existing note. Large changes to allocation, deleting
energetic modes, or jumping between widely different textures can still be
audible edits; state retention is not a promise that such changes sound like
an untouched note. Saved patches and new renders always use the normal engine
preparation, with no editor-only synthesis mapping.

## Tests

`percussion_modal_live_tests` checks pitch changes during an existing tail,
no-op phase/drift/shimmer continuity, identity reordering and packet-energy
conservation during allocation changes. `percussion_modal_retirement_tests`
checks sample-step continuity and fade/reset/mute behaviour separately.
Host/UI tests cover held drags, Escape rollback for all modal tools, undo/redo,
latest-update coalescing during retirement and preset/rate cancellation. The standalone
allocation test watches the real callback for both allocation and freeing while
prepared edits are adopted. Mailbox publication is stressed concurrently.
