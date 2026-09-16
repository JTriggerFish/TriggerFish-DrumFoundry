# Direct strike accent

Metallic-plate instruments can add a short noisy contact directly to their
observation mix. This is a constructive attack layer, not a model of the
sustained metallic sizzle or of cymbal-to-cymbal collisions.

```text
                      existing contact → modal body → bloom/ringing
strike gesture ──────<
                      direct contact + noise accent → observation mix → final EQ
```

The existing contact, modal bank, transport, damping and tuning are unchanged.
The accent has no body-drive port. Changing it cannot affect stored modal
energy, even on repeated hits. It follows the existing contact-to-observation
route and the final output EQ/master; it is not sent through Contact presence
(which remains the gain for the original direct contact).

## Visible controls

- **Noise accent level** (`contact_noise_level`): linear amplitude, 0–2.
  Zero is off and is the default; existing presets render unchanged.
- **Noise accent decay** (`contact_noise_decay`): 2–120 ms T60. A 0.2 ms
  smooth onset prevents an instantaneous envelope step; there is no added
  scheduling delay or hold. The source stops at −80 dB.
- **Noise accent brightness** (`contact_noise_colour`): −24 to +24 dB broad
  complementary shelf tilt around 4.2 kHz. This is not dB/oct or resonant EQ.

All three belong to the observation node in JSON and appear in **Strike accent**
on the left. They support host automation/live editing: level is smoothed,
while decay and brightness are captured by the next strike. Velocity scales
the noise amplitude linearly. There is no auto-level matching, hidden limiter,
or modification of the existing implement-dependent excitation.

The noise uses the existing `EnvelopedNoiseBurst` primitive, an independent
seed stream and eight fixed overlapping envelopes. On pathological overflow
only the new accent is omitted; no sounding envelope or body strike is cut.
There is no audio-thread allocation. This is a tiny source, not a second
instrument render or crossfade.

## Verification

Tests cover exact body-state isolation, observation routing, overlapping
envelopes, measured T60, linear velocity and reset at 44.1/48/96 kHz. Generic
live-parameter tests compare the next strike against freshly loaded JSON.
The source hi-hat preset is compared against its frozen pre-change native
render. Audition presets stay local, separate from factory presets.

Short contact colour alone cannot repair a sustained body-spectrum mismatch.
Assess the attack before fitting the later high-mid zing; don't improve an
attack score by changing the resonator tuning or decay behind the user's back.
