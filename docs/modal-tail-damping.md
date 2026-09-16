# Modal tail damping (experimental)

`body_decay_friction`, **Tail damping**, is visible below the metallic voice's
T60 editor. It defaults to zero, is saved in the body node, and supports live
edits and CLAP automation. Existing presets are unchanged at zero.
The slider uses a cubic taper for finer adjustments near zero; displayed and
serialized values remain the actual loss rate, not the slider position.

## Why try this?

An exponential amplitude envelope has a constant slope in dB. Some reference
tails sustain initially and then fall more steeply. Shortening T60 alone cannot
produce both. That observation does **not** establish that each physical mode
has a threshold: beating, loss of collision excitation, microphone noise and
sample editing can also affect the measured envelope.

Sekiguchi and Samejima's hi-hat model explicitly reports an overlong late tail
when collision losses stop acting; they discuss missing static contact pressure
as an explanation. It does not demonstrate Coulomb friction in fully separated
cymbals. Separately, dry-friction oscillator theory gives approximately linear
amplitude decay and a finite stopping time. We borrow that *envelope behaviour*
as a constructive damping model, not a complete simulation of cymbal contact.

## Signal and energy rule

Each mode stores a complex quadrature state with amplitude $A_i=|z_i|$.
The T60 curve gives ordinary viscous loss $\lambda_i=\ln(1000)/T_{60,i}$.
The new control adds a constant amplitude drain:

$$
\frac{dA_i}{dt}=-\lambda_i A_i-c|w_i|,\qquad A_i\geq0.
$$

Here $c$ is the visible normalized amplitude loss per second and $w_i$ is
the mode's prepared, normalized input weight. The unforced solution is

$$
A_i(t)=\max\left(0,
 \left[A_i(0)+\frac{c|w_i|}{\lambda_i}\right]e^{-\lambda_i t}
 -\frac{c|w_i|}{\lambda_i}\right).
$$

Implementation uses a first-order operator split: ordinary modal propagation,
then radial shrinkage by $c|w_i|/f_s$, then passive energy exchange. Both real and
imaginary state components get the same nonnegative gain. This removes energy,
preserves phase and never hard-clips an oscillating waveform. There is no output
gate, peak-memory threshold, per-strike timer or dead-mode latch. Excitation or
energy exchange can revive a silent mode. Removed modes still receive the normal
5 ms edit retirement fade; this is not part of the physical decay model.

Weighting matters: splitting one state into $N$ equal-energy states divides both
its amplitude and its drain by $\sqrt N$. Merely increasing density must not
shorten the decay. This is exact for equivalent subdivisions; changing spectral
placement, input distribution or nonlinear exchange can still change decay.
Observation gain does not affect the drain. Softer strikes naturally end sooner;
there is no velocity compression or output normalization.

Use a longer T60 with modest Tail damping for a sustained sound that ends more
decisively. T60 is now the *linear damping component*, not a promise that the
complete nonlinear voice takes that exact time to fall 60 dB. Large damping
values can suppress soft strikes and bloom: this is a real trade-off, not a
hidden compensation. The control is smoothed over 5 ms during live edits.

## Evaluation

Compare T60-only fitting against T60 plus this one parameter. Freeze the strike,
mode placement, texture, observation gains and EQ. Measure decay shapes above the
reference noise floor, separately from absolute level. A shape comparison may
subtract a fixed early-band level for analysis, but never normalize rendered
audio. Inspect several reference takes before attributing their final fades to
physical damping. Check lower velocities, multiple random seeds and restrikes;
do not accept an improved single-hit score as proof of better timbre.

Tests cover zero bypass, phase retention, passivity, finite extinction, revival,
live layout retention, and the analytic mixed-loss envelope across 44.1/48/96 kHz
and 1/8/64/512 equal-energy states. The radial-loss primitive lives in
`engine/tfdsp/percussion/modal_friction_loss.hpp`.

## References

- Sekiguchi & Samejima (2023), [Physical modeling and sound synthesis of the
  hi-hat](https://www.jstage.jst.go.jp/article/ast/44/5/44_E2293/_pdf), especially §4.3.
- Molina (2004), [Exponential versus linear amplitude decay in damped
  oscillators](https://arxiv.org/abs/physics/0407080), *The Physics Teacher* 42,
  485–487, DOI 10.1119/1.1814324.
