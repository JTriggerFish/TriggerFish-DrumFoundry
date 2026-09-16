# Modal energy diffusion

`ModalSpectralDiffusion` redistributes **stored energy**, not oscillator
frequencies. Packet centres define a finite-volume grid in $x=f/1000$;
coincident centres share a cell. Cell widths are $w_i$. Packet energy is
normalized by the prepared unit-strike reference energy for this calculation
only; this does not normalize the audio or its stored energy.

Let $e_i$ be normalized cell energy, $u_i=e_i/w_i$ its density, and
$E=\sum_i e_i$. The conductance between neighbouring cells is

$$
k_i=h\,g_i E^s
\left(\frac{a_i^2+a_i a_{i+1}+a_{i+1}^2}{3}\right)^p,
\qquad a_i=\frac{u_i}{E}.
$$

Here $h$ is diffusion strength divided by sample rate, $g_i$ is the
cell-face frequency divided by centre spacing, $p$ is concentration dependence,
and $s$ is energy sensitivity. A zero exponent contributes a factor of one.
Thus concentration dependence changes the response to spectral shape, while
energy sensitivity changes the response to total strike energy.

Conductances are frozen at the old state. One tridiagonal solve computes

$$
(w_i+k_{i-1}+k_i)u'_i-k_{i-1}u'_{i-1}-k_i u'_{i+1}=e_i.
$$

The outer boundary fluxes are zero. The matrix is an M-matrix: its inverse is
nonnegative, and summing its equations conserves total energy, up to rounding.
The cancellation-resistant Thomas solve avoids iterative convergence work.
This remains a semi-implicit numerical model, not an exact solution of a
physical cymbal equation.

`ModalEnergyCascade` distributes each resulting cell energy back into its
packet. Existing within-packet proportions are retained; an empty packet uses
its prepared excitation weights. State amplitudes are rescaled by square-root
energy ratios. Packet phase handling is separate from this energy solve.
In each sample, modal propagation applies the declared T60 damping before
the cascade redistributes the remaining energy.

The reusable cascade primitive also retains a tested one-way transport option;
it is a distinct algorithm, not the diffusion equation above. This document
describes the nonlinear diffusion implementation. It is not a claim that the
model reproduces every physical energy-transfer mechanism in metal percussion.

Tests in `tests/dsp/percussion_spectral_diffusion_tests.cpp` cover passivity,
nonnegativity, coincident centres and discretization behaviour.
