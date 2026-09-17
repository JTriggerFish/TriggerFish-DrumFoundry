# Hi-hat rim contact

This is a **reduced contact interaction attached to a modal body**, not
a second complete cymbal voice and not an added noise envelope. It is optional
and defaults off at the component level. The factory **Hi-hat** enables it and
is the startup preset for new plugin instances and standalone launches.
Saved DAW projects still restore their saved instruments. Factory presets carry
no sample reference; the accepted reference-linked calibration stays local.
Presets with contact disabled retain their sound. The original strike
accent remains a separate, explicitly direct-only contact sound.

The same module can now attach to metallic or membrane resonators via the
[topology editor](topology-modules.md), not just a hi-hat preset.

## Controls and routing

The visible **Rim contact** group has seven controls, all serialized and live:

| Control | Meaning |
| --- | --- |
| Enable rim contact | Exact bypass when off |
| Separation / pedal | 0 closed, 1 open; also MIDI CC4, with 127 closed and 0 open |
| Open clearance | Maximum normalized relative-rim gap, not millimetres |
| Contact damping | Fraction of relative kinetic energy lost in each impact; also resting contact drag |
| Pedal strength | External closing-velocity coupling, independent of stick hits |
| Rattle motion | Correlated changes in contact participation and local touching/separating |
| Settling | Slow to fast rattle returns and settling; zero is the slowest setting |

The last two default to zero. Settling is no longer an algorithm switch at zero:
the same rattle model runs throughout its range. Disabled contact bypasses
everything. Neither control changes modal
centre frequencies, adds audio noise or runs an elapsed-since-hit envelope.

The modal body first advances its oscillators, intrinsic losses and energy
diffusion. Rim contact then updates the same modal velocity states. Existing
neighbour exchange and observation follow it. There is no second audio path,
automatic gain compensation, independent sizzle source or delayed strike.
Hand mute continues to use the existing passive modal loss controller and
never supplies energy. Contact does not replace the ordinary T60 curve.

## Reduced coordinates

For each mode, the real quadrature is a mass-normalized velocity $v_i$ and the
imaginary quadrature is $s_i=\omega_i q_i$. Four signed rim participation vectors
$w_{pi}$ define relative displacement and velocity at contact ports:

$$
q_p=\sum_i w_{pi}s_i/\omega_i,\qquad v_p=\sum_i w_{pi}v_i.
$$

The vectors use a deterministic spatial cosine basis indexed by stable packet
identity, weighted by prepared modal allocation. Satellites inherit their
handle's spatial participation. Each vector is normalized in squared mass
coordinates; splitting an identical mode into equally weighted members does
not change contact strength. These are constructive spatial coordinates, not
measured shell eigenfunctions. Four is the fixed spatial discretization, not
a hidden fitted sound parameter.

Clearance is expressed in velocity-milliseconds: the physical calculation
uses $g=0.001\,c\,o^3$. The factor only defines units; cubic travel reserves
more pedal range for small grazing-contact gaps. This is a control mapping,
not a claim about a particular pedal linkage. Openness is smoothed over
5 ms using the same control smoother as the other live parameters. Its actual
sample-to-sample motion defines the gap velocity; repeated MIDI
values do not restart that motion. Loading, resetting or enabling a patch sets
the pedal position without imparting work. Editing clearance is a geometry
edit, not a simulated foot movement. The explicit **pedal strength** adds a
closing-only effective actuator velocity $-0.001\,p\,r^2/(4+r)$, where
$r=\max(-\dot o,0)$ in full pedal strokes per second. The four-stroke/second
knee makes the response quadratic near rest and approximately linear for fast
closure. This explicitly suppresses continuous excitation from slow closing;
it does not alter the stick velocity curve. The sum of
gap and actuator velocities is $v_g$ in the impact law below. This separates
the external foot-energy scale from the small vibrating-rim gap. It is a reduced
drive port, **not** a kinematically exact moving shell: $v_g$ need not equal
$\dot g$ when pedal strength is nonzero. Both the energy supplied and the
contact loss are accounted for. There is no triggered noise burst or latch.

## Impact and energy budget

At an overlapping, approaching port ($q_p\geq g$, $u=v_p-v_g>0$), restitution
is $e=\sqrt{1-\ell}$ for visible contact loss $\ell$. Apply the impulse

$$
k_p=\sum_iw_{pi}^2,\qquad
J=-\frac{(1+e)u}{k_p},\qquad v_i'=v_i+w_{pi}J.
$$

Displacement is not clipped or warped. With stored energy
$E=\tfrac12\sum_i(v_i^2+s_i^2)$, the exact change is

$$
E'-E=\underbrace{Jv_g}_{\text{pedal work}}
-\underbrace{\frac{(1-e^2)u^2}{2k_p}}_{\text{dissipation}}.
$$

Thus fixed-pedal contact cannot create energy. It can redistribute energy into
previously quiet resonators while losing some of it. A closing pedal can do
positive work; slow closure supplies much less than fast closure. No trigger
threshold, one-shot latch or oscillator reset is used. Several sequential
ports preserve the same budget. Note that the existing field diagnostic
`StoredEnergy()` uses **twice** the convention above.

## Scope and limitations

This is an experiment, not a validated physical two-shell simulation. The
participation vectors and relative displacement approximation need listening
and calibration. Dispersive packet modulation is not a measured rim trajectory.
Impacts are evaluated at the audio sample rate: passivity proves boundedness,
**not** absence of aliasing or sample-rate-independent collision timing.

In the original fixed-wall experiment, a positive gap eventually stopped
collisions as motion became too small. This could not explain the abrupt ending
of every open-hat resonance. The rattle model below is a constructive alternative, not measured
shell geometry, air flow, or a full model of static clamping and washers.

## Perceptual rattle and settling extension

Each of the four contact coordinates can exchange energy with one unit-mass
internal rattle state. Its velocity is $b$ and its nonnegative stored potential
is $U$. It is silent by itself; only impulses returned to the modal body sound.
With smoothed settling $s$ and openness $o$, return acceleration is
$a=0.01+0.99s^2$ and coupling is $c=o$. The explicit acceleration range is
100:1; its square mapping gives the slow region more slider space. Settling
controls return force, not coupling/mass. Zero is a slow but finite return,
not a bypass or a closed boundary. Only closing the pedal removes the freedom
to bounce, becoming a fixed dissipative boundary. Both controls use the existing
5 ms live-edit ramp. Potential is
stored as energy, not height: changing settling changes travel without creating
potential energy. The inferred height is $U/a$.

Free travel integrates constant acceleration exactly up to the next impact:

$$
E_r=\tfrac12 b^2+U,\qquad b'=b-a\Delta t,\qquad
U'=E_r-\tfrac12 b'^2.
$$

On return to $U=0$, the incident speed is $-\sqrt{2E_r}$. Exchange with the
body uses $u=v_p-v_g-cb$, denominator $k_p+c^2$, and equal reciprocal changes
$v_i'=v_i+w_{pi}J$, $b'=b-cJ$. The same work-minus-loss identity applies to
the **combined** modal and rattle energy. Returning events can become closer
together as energy falls; no accelerating impulse clock is imposed.

Returns are resolved at the next audio sample, so timing is quantized by at
most one sample. Bounces shorter than the time step enter resting contact;
their remaining energy is explicitly dissipated. This avoids unresolved
infinite chatter but is not proof of alias-free output. Further strikes and
pedal closure can excite the state again: there is no dead-state latch.

At rest and while touching, a Coulomb-like impulse opposes relative velocity.
Here the boundary velocity is the actual pedal gap motion only: the extra
pedal-strength actuator belongs to impacts and must not turn slow closure into
a continuously driven friction sound.
Its magnitude is limited by $a\ell\Delta t$ and by the impulse needed to reach
zero relative velocity. This cannot reverse motion or add energy. It can stop
the contact-participating motion decisively, without gating the whole output.
Uninvolved modes may continue ringing under their ordinary T60 losses.

**Rattle motion** uses the existing smooth random modulator at a 32-sample
control interval. Four deterministic streams (base rates 2, 2.3, 2.6 and 2.9 Hz)
blend adjacent normalized rim participation vectors. If the blend is $m$, the
local gap is $g(1-2m)$: it can move from separation into contact. Rates/seeds
are fixed texture-algorithm constants, not privately fitted parameters. This
bounded control trajectory is not an additional audio source or stored-energy
reservoir; every impulse is passive for its current participation vector.

Limitations: returning motion stays associated with the contacted patch rather
than reconstructing two shell surfaces. Local rocking is a perceptual control
trajectory, not a physically powered rigid-body solver. The rattle state has
correct energy accounting, but that alone does not validate its timbre. Motion,
settling and contact damping necessarily interact through collision activity.
Neither openness nor settling edits reset returning motion, including at zero.
Disabling contact discards its stored energy, never adding it to the output.

The former $a=s^2$, $c=so$ mapping and explicit zero bypass made the bottom of
Settling act like a sudden closure. Removing that mass change and algorithm
switch is intentional: older contact-enabled experiments near zero change sound.
No extra user parameter, output crossfade or gain compensation is involved.

## Reference check for this experiment

In the local open-hat reference, the energy-weighted centre of the 2--20 kHz
region stays near 9.7--10.1 kHz from 0.1 to 1.2 seconds, before dropping during
the ending. The highest band falls about 26 dB between the 1.2--1.4 and
1.6--1.8 second windows. These are measurements of one recording, not universal
hi-hat targets or proof that its rattle repetition rate rises. The experiment
targets a bright, changing tail followed by rapid disappearance, not a modal
pitch sweep. Keep recordings, analysis artifacts and calibration fits local.

## Tests and fitting procedure

`percussion_rim_contact_tests` checks the energy budget at 44.1/48/96 kHz,
elastic redistribution, packet subdivision, exact bypass, quiet slow closure,
closed-reset silence and restriking. Host tests check CC4 and live publication;
`percussion_rattle_tests` adds combined-state energy budgets, moving contact,
accelerating inelastic returns, finite settling and silence checks. The modal
field energy diagnostic includes twice the rattle energy to match its existing
quadrature convention. Tests must not mistake energy returned from the rattle
state for energy created in the modal body.
Python's `Renderer.set_parameter` uses that same native scalar control path.
Zero-endpoint regressions compare 0 with 0.001 on fresh strikes and during a
live tail. Native energy-budget checks include crossing zero without clearing
the rattle state at 44.1/48/96 kHz. In the accepted local moving-rattle preset,
the zero-to-0.001 change stays within 0.06 dB across six half-second energy
windows; the former mapping collapsed the tail. These are continuity checks,
not proof of perceptual equivalence between different settling settings.
Geometry controls requiring preparation are rejected by this Python method.
The source document remains unchanged, so save explicit parameter changes in
the fitting layer when creating a preset.

Start from an accepted open preset. Preserve its modal series, strike and
texture. Test contact gap/loss with shared broad damping refinements against
both accepted open and half-open sounds at a common strike, then compare real
references separately. A smaller loss score is not sufficient acceptance:
inspect time-frequency differences, early tone, inter-ridge texture, tail
endings, velocity response and repeated/pedal-only gestures. Keep experimental
fits separate from the user's originals and out of factory presets.

For the moving-rattle audition pass, the saved open-hat mode placement, packet
texture and strike were held fixed. Search varied settling, contact damping,
and two broad T60 multipliers blended over log frequency from 1 to 16 kHz.
Rattle motion was held at 0.7. The diagnostic objective compared four band
spectral proportions, decay relative to the early window, and the late decay
drop against the real reference. It did not optimize modal positions or output
EQ. Audio was never normalized; the candidate has an explicit, fixed -6 dB
output trim relative to its parent for audition headroom.

This is an experimental candidate, not a perceptually validated calibration.
The high-band ending is closer in the inspected spectrogram, but upper spectral
balance remains different. Test motion off and settling at its slow endpoint by
setting each to zero in the same saved candidate. Rate/velocity checks cover 44.1, 48
and 96 kHz, three strike strengths, open/half/closed, restrikes and fast/slow
pedal-only closures. They are robustness checks, not an alias-free guarantee.

## References

- Sekiguchi & Samejima (2023), [Physical modeling and sound synthesis of hi-hat](https://doi.org/10.1250/ast.44.352).
  Their colliding-shell models motivate jointly treating redistribution and
  loss. Their velocity-exchange model is not identical to this modal projection.
  They also discuss missing static contact pressure and support/tilt dynamics;
  their high simulation rate must not be mistaken for an audio-rate guarantee.
- Bilbao, Torin & Chatziioannou, [Numerical Modeling of Collisions in Musical
  Instruments](https://arxiv.org/abs/1405.2589). Energy accounting motivates the
  passivity tests; this implementation is an impulse projection, not their
  nonlinear potential solver.
