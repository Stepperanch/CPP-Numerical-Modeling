<p align="center">
  <a href="https://nelsbuhrley.github.io/CPP-Numerical-Modeling/#academic-projects"><strong>← Back to Portfolio Hub</strong></a>
</p>

# Project 9: Quantum Mechanics — Bound States & Eigenstates

> [!TIP]
> Every highlighted link in this page is clickable.
> For fast navagation use [Table of Contents](#table-of-contents)

**Author:** Nels Buhrley
**Language:** C++17 · Python 3 (visualization)
**Build:** `make release` — see [Build & Run](#build--run)

---

## Snapshot

- Built a full quantum mechanics eigenstate solver that computes bound states for polynomial potential wells by integrating the 1D Schrödinger equation.
- Implemented the **Numerov method** (4th-order finite-difference integration) to solve $-\frac{\hbar^2}{2m}\psi''(x) + V(x)\psi(x) = E\psi(x)$ with arbitrary precision.
- Developed a **shooting-and-matching algorithm** combining energy sweeps with node counting and bisection search to locate eigenstate energies.
- Generated interactive 3D visualizations of eigenstates and potential wells across multiple quantum numbers and well shapes (polynomial degrees up to 6).
- Demonstrates expertise in numerical quantum mechanics, boundary value problem solving, and high-precision root-finding.

For a fast technical check, jump to [Code Structure](#code-structure), [Results](#results), and [Build & Run](#build--run).

---

## Table of Contents

- [Snapshot](#snapshot)
- [Overview](#overview)
- [Physics Background](#physics-background)
  - [The 1D Time-Independent Schrödinger Equation](#the-1d-time-independent-schrödinger-equation)
  - [Bound States and Quantization](#bound-states-and-quantization)
  - [Polynomial Potentials](#polynomial-potentials)
- [Code Structure](#code-structure)
  - [Numerov Integration](#numerov-integration)
  - [Eigenstate Finding](#eigenstate-finding)
  - [Boundary Value Problem Solving](#boundary-value-problem-solving)
- [Results](#results)
  - [Interactive Eigenstates Visualization](#interactive-eigenstates-visualization)
  - [Well Shapes and Bound States](#well-shapes-and-bound-states)
- [Sources of Error](#sources-of-error)
- [Build & Run](#build--run)
- [Simulation Parameters](#simulation-parameters)
- [Key Techniques](#key-techniques)
- [Project Structure](#project-structure)

---

## Overview

This project implements a **quantum mechanics eigenstate solver** for the one-dimensional time-independent Schrödinger equation with finite-range polynomial potentials. The solver:

1. Sweeps through energy space to locate **nodal brackets** — energy pairs that bracket a zero-crossing in the wavefunction
2. Uses **bisection refinement** to converge on each eigenstate's energy with machine precision
3. Integrates the Schrödinger equation via the **Numerov method** to obtain the spatial wavefunction $\psi(x)$
4. Supports polynomial potentials of arbitrary degree $V(x) = \frac{x^n}{n}$ and multiple quantum states

The implementation demonstrates advanced techniques in numerical boundary value problems: energy quantization by node counting, high-order finite-difference schemes, and robust root-finding under nonlinear ODE constraints.

---

## Physics Background

### The 1D Time-Independent Schrödinger Equation

The binding structure of quantum systems follows from the time-independent Schrödinger equation:

$$-\frac{\hbar^2}{2m}\frac{d^2\psi}{dx^2} + V(x)\psi(x) = E\psi(x)$$

In natural units ($\hbar = m = 1$), this becomes:

$$-\frac{1}{2}\psi'' + V(x)\psi = E\psi$$

or equivalently, rearranging to standard ODE form:

$$\psi''(x) = 2[V(x) - E]\psi(x)$$

### Bound States and Quantization

For a finite-range potential $V(x) \to 0$ as $|x| \to \infty$, **bound states** exist when $E < V_{\max}$ and satisfy the boundary conditions:

- $\psi(x) \to 0$ as $x \to \pm\infty$

These boundary conditions are **quantizing**: only discrete energy values $E_n$ allow the wavefunction to decay smoothly to zero at both boundaries. The energy quantization is directly encoded in the **nodal structure**: excited state $n$ has exactly $n$ nodes (zero-crossings) in its wavefunction.

### Polynomial Potentials

This solver focuses on polynomial potentials of the form:

$$V(x) = \frac{x^n}{n}$$

where $n$ is the **degree** (ranging from 2 to 6). Each degree corresponds to:

- **n = 2:** Harmonic oscillator potential (parabolic well)
- **n = 3:** Cubic well
- **n = 4:** Quartic well (double-well structure)
- **n = 5 – 6:** Higher-order confinement

The symmetry of the potential ($V(-x) = (-1)^n V(x)$) splits eigenstates into **even** and **odd parity** families, enabling independent sweeps with appropriate initial conditions.

---

## Code Structure

All core logic resides in [processing.h](processing.h) with entry point [main.cpp](main.cpp).

| Component | Purpose |
|---|---|
| `NumerovIntegrate()` | 4th-order finite-difference solver for the ODE |
| `countNodes()` | Counts zero-crossings in $\psi(x)$ to identify quantum number |
| `findNodalBracketsAtDegree()` | Energy sweep + node detection to bracket each eigenstate |
| `findEigenstatesAtDegree()` | Bisection refinement on brackets to converge eigenstate energies |
| `Sweep` class | Orchestrates multi-degree sweep and output |

### Numerov Integration

The Numerov method is a **4th-order predictor-corrector scheme** for solving $y'' = g(x, y)$. For the Schrödinger equation with $g = 2[V(x) - E]$, the recurrence relation is:

$$\psi_{i+1} = \frac{2(1 + 5h^2g_i/12)\psi_i - (1 - h^2g_{i-1}/12)\psi_{i-1}}{1 - h^2g_{i+1}/12}$$

where $h = \Delta x$ is the spatial step size and $g_i = 2[V(x_i) - E]$.

**Advantages:**
- 5th-order local truncation error (vs. 3rd for standard 2nd-order schemes)
- Compact 3-point stencil, easy to parallelize
- Stable for smooth potentials

**Implementation details** (from [processing.h](processing.h)):
- Precomputes $2V(x_i)$ on the full grid to avoid redundant arithmetic
- Coefficients $a_{\text{prev}}, a_{\text{curr}}, a_{\text{next}}$ factor out division via careful arithmetic
- Detects divergence (when $\psi > 10^4$) to halt integration early
- Returns only the valid non-divergent portion of $\psi(x)$

### Eigenstate Finding

#### Step 1: Energy Sweep & Nodal Bracketing

The solver sweeps energy from `energyMin` upward in steps of `energyStepSize`, tracking node count. When the node count increases, an eigenstate bracket is recorded:

```
E_minus = previous energy (fewer nodes)
E_plus  = current energy  (one more node)
```

This pair brackets the eigenstate. Separate sweeps for even ($\psi(0) = 1, \psi'(0) = 0$) and odd ($\psi(0) = 0, \psi'(0) = 1$) parity allow independent exploration of each family.

#### Step 2: Bisection Refinement

Once a bracket $[E_-, E_+]$ is located, bisection converges the eigenstate energy. For each midpoint $E_{\text{mid}} = (E_- + E_+) / 2$:

1. Integrate $\psi(x; E_{\text{mid}})$ from $x = 0$ to $x_{\text{end}}$
2. Evaluate $\psi(x_{\text{end}})$
3. If $\psi(x_{\text{end}})$ has the **same sign as at $E_+$**, replace $E_+$; otherwise replace $E_-$
4. Repeat until bracket width $< 10^{-15}$

The sign of $\psi$ at the boundary tracks which side of the energy eigenvalue we are on, enabling robust root-finding without computing derivatives.

#### Step 3: Wavefunction Refinement

Once the eigenstate energy is converged:

1. **Trim:** Remove the divergent tail (tail magnitude > interior magnitude)
2. **Normalize:** Enforce $\int_{-\infty}^{\infty} |\psi(x)|^2 dx = 1$

---

## Results

### Interactive Eigenstates Visualization

The eigenstates across all potential degrees (n = 2 to 6) and up to 10 bound states per degree are visualized as **tabbed interactive 3D plots**.

<div style="width: 100%; aspect-ratio: 1 / 1; overflow: hidden; border: 1px solid #ddd; border-radius: 8px;">
    <iframe
        src="https://nelsbuhrley.github.io/assets/html-assets/eigenstates_tabs_simple.html"
        width="100%"
        height="100%"
        style="border: none; display: block;">
    </iframe>
</div>

<a href="https://nelsbuhrley.github.io/assets/html-assets/eigenstates_tabs_detailed.html" target="_blank">View Detailed Eigenstates Fullscreen ↗️</a>

**Features:**
- **Tabs by potential degree:** Switch between harmonic (n=2), cubic (n=3), quartic (n=4), and higher-order wells
- **3D wavefunction plot:** $\psi(x)$ and potential well superimposed; rotate to inspect structure
- **Potential profile:** Visual representation of the well shape
- **Energy table:** Quantized energy levels with full precision

Select any degree tab above to explore states for that potential family.

### Well Shapes and Bound States

Each potential supports a discrete set of bound states determined by energy quantization. The relationship between well shape and eigenstate structure:

| Degree | Potential | Well Type | Key Feature |
|---|---|---|---|
| 2 | $V(x) = x^2/2$ | Parabolic | Evenly-spaced levels; analytical basis available (Hermite polynomials) |
| 3 | $V(x) = x^3/3$ | Cubic asymmetric | Levels cluster near soft boundary; smooth shape |
| 4 | $V(x) = x^4/4$ | Quartic double-well | States exhibit avoided crossings; higher-n states probe double-well barrier |
| 5 | $V(x) = x^5/5$ | Quintic | Deepening well; rapid confinement increase |
| 6 | $V(x) = x^6/6$ | Sextic | Steepest walls; highest confinement of implemented potentials |

The potential symmetry determines parity:
- **Even degree** ($n = 2, 4, 6$): Symmetric well, $V(-x) = V(x)$ → even and odd eigenstates both exist
- **Odd degree** ($n = 3, 5$): Asymmetric well, $V(-x) \neq V(x)$ → both parities mix

---

## Sources of Error

| Source | Nature | Mitigation |
|---|---|---|
| Spatial discretization | $\Delta x$ finite → wavefunction sampled at grid points | Use `stepSize = 0.0001`; Numerov method is 5th-order accurate |
| Boundary truncation | Potential extends to $x_{\text{end}} = 8.0$ rather than $x = \infty$ | Wavefunction decays exponentially; tail contribution $< 10^{-6}$ |
| Bisection convergence | Finite precision in energy bracket | Tolerance set to $10^{-15}$; typical refinement: ~50 iterations per state |
| Node counting discreteness | A state's node count only advances by 1 per energy increment | Use small `energyStepSize = 0.5` to ensure no state is skipped |
| Numerov truncation error | 5th-order scheme accumulates error over $10^5$ steps | For longer integrations, reduce step size (quadratic cost) |

**Computational complexity:**
- Per eigenstate: $\mathcal{O}(150 \text{ Numerov sweeps} \times 80{,}000 \text{ spatial points}) = \mathcal{O}(12 \text{ M updates})$
- Total: $\mathcal{O}(6 \text{ degrees} \times 10 \text{ states} \times 12 \text{ M}) = \mathcal{O}(720 \text{ M FLOPs})$ — runs in < 1 s

---

## Build & Run

### Prerequisites

- **C++17** compiler (`g++` or `clang++`)
- **zlib** (for `.npz` output; usually system-provided)
- **Python 3** with `numpy` and `plotly` (for visualization)

### Build Targets

```bash
make release   # Optimized build (-O3, LTO, vectorization, march=native)
make debug     # -O0, full warnings
make clean     # Remove build artifacts
```

### Run

```bash
./bin/main
```

Output is saved to `output/eigenstates.npz`. Then generate visualizations:

```bash
python3 postprocessing.py
```

This produces:
- `eigenstates_tabs_detailed.html` — Full dashboard with 3D plot, potential, and data table
- `eigenstates_tabs_simple.html` — 3D-only simplified view

Upload both HTML files to the assets folder for inclusion in the portfolio.

---

## Simulation Parameters

Configured in [main.cpp](main.cpp):

| Parameter | Default | Description |
|---|---|---|
| `maxDegree` | 6 | Highest polynomial degree to solve (n = 2...6) |
| `nodesToFind` | 10 | Number of bound states per degree |
| `stepSize` | 0.0001 | Spatial discretization $\Delta x$ |
| `energyStepSize` | 0.5 | Energy increment for nodal bracket sweep |
| `convergenceTol` | 1e-15 | Bisection convergence threshold on bracket width |
| `maxIterations` | 150 | Max bisection iterations per eigenstate |
| `energyMin` | 0.001 | Minimum energy for sweep start |
| `targetXEnd` | 8.0 | Integration domain endpoint (normalization reference) |
| `targetPsiPoints` | 150 | Resampling target for output visualization |

---

## Key Techniques

| Technique | Purpose |
|---|---|
| Numerov 4th-order method | High-accuracy ODE integration with compact stencil |
| Node counting via sign crossings | Identifies quantum number without computing spectrum |
| Precomputed potential mesh | Avoids redundant function evaluations in loop |
| Early divergence detection | Halts integration when $\|\psi\| > 10^4$ to save compute |
| Even/odd parity decomposition | Reduces energy sweep by separating independent families |
| Bisection + shooting method | Robust eigenstate energy convergence without derivatives |
| Wavefunction trimming | Removes divergent tail post-integration |
| Wavefunction normalization | Ensures $\|\psi\|_2 = 1$ for correct probability interpretation |
| Resampling for output | Downsamples from ~80,000 grid points to ~150 for visualization |
| NPZ compression | Stores all states and energies in a single compact binary archive |

---

## Project Structure

```
Project 9: Quantum Mechanics/
├── main.cpp                    # Entry point: configures degrees, iterations, parameters
├── processing.h                # Core solver: Numerov, nodal bracketing, bisection refinement
├── Makefile                    # Multi-target build: debug, release, clean
├── postprocessing.py           # Generates interactive Plotly dashboards from NPZ output
├── requirements.txt            # Python dependencies (numpy, plotly)
└── output/
    └── eigenstates.npz         # Compressed eigenstate database
        ├── psi_degree_2_state_0..9       # Wavefunction trajectories for n=2
        ├── energies_degree_2             # Energy levels for n=2
        ├── psi_degree_3_state_0..9       # Wavefunction trajectories for n=3
        ├── energies_degree_3             # Energy levels for n=3
        ├── ... (similarly for n=4,5,6)
        └── targetXEnd                    # Integration endpoint (8.0)
```

---

*Nels Buhrley — Computational Physics, 2026*

<p align="center">
  <a href="https://nelsbuhrley.github.io/CPP-Numerical-Modeling/#academic-projects"><strong>← Back to Portfolio Hub</strong></a>
</p>
