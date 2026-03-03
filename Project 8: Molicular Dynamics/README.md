# Project 8: Molecular Dynamics

**Author:** Nels Buhrley
**Language:** C++17 with OpenMP · Python 3 (visualization)
**HPC:** Run on the BYU Supercomputer (128 CPUs via SLURM)
**Build:** `make release` — see [Build & Run](#build--run)

---

## Overview

This project implements a **2D Molecular Dynamics simulation** of interacting particles governed by the **Lennard-Jones potential**. The system evolves via the **velocity Verlet integration** algorithm with periodic boundary conditions, modeling the thermodynamic behavior of a simple fluid. An external **energy injection/extraction schedule** drives the system through heating and cooling phases, allowing observation of phase-like transitions in real time.

The implementation combines **OpenMP parallelism** for force calculations, **minimum image convention** for periodic boundaries, and a **cutoff radius** for efficient pairwise interaction evaluation. Results are saved as `.npz` and `.csv` files, with a Python pipeline that produces an animated particle trajectory and energy analysis plots — rendered in parallel segments and stitched with `ffmpeg`.

---

## Physics Background

### The Lennard-Jones Potential

Particles interact via the Lennard-Jones (12-6) potential in reduced units ($\sigma = \varepsilon = m = 1$):

$$U(r) = 4\varepsilon\left[\left(\frac{\sigma}{r}\right)^{12} - \left(\frac{\sigma}{r}\right)^{6}\right]$$

The steep $r^{-12}$ repulsion models short-range Pauli exclusion, while the $r^{-6}$ attraction captures van der Waals dispersion forces. A cutoff radius $r_c = 2.5\,\sigma$ truncates the potential for computational efficiency — contributions beyond $r_c$ are negligible.

### Equations of Motion

Each particle obeys Newton's second law:

$$m\,\ddot{\mathbf{r}}_i = \mathbf{F}_i = -\nabla_i \sum_{j \neq i} U(r_{ij})$$

The force on particle $i$ due to particle $j$ is:

$$\mathbf{F}_{ij} = \frac{24\varepsilon}{r^2}\left[2\left(\frac{\sigma}{r}\right)^{12} - \left(\frac{\sigma}{r}\right)^{6}\right]\mathbf{r}_{ij}$$

Newton's third law ($\mathbf{F}_{ij} = -\mathbf{F}_{ji}$) halves the number of pair evaluations.

### Velocity Verlet Integration

The simulation advances positions and velocities using the velocity Verlet scheme, which is time-reversible and symplectic:

$$\mathbf{v}_i\left(t + \tfrac{\Delta t}{2}\right) = \mathbf{v}_i(t) + \tfrac{\Delta t}{2}\,\mathbf{a}_i(t)$$

$$\mathbf{r}_i(t + \Delta t) = \mathbf{r}_i(t) + \mathbf{v}_i\left(t + \tfrac{\Delta t}{2}\right)\,\Delta t$$

$$\mathbf{v}_i(t + \Delta t) = \mathbf{v}_i\left(t + \tfrac{\Delta t}{2}\right) + \tfrac{\Delta t}{2}\,\mathbf{a}_i(t + \Delta t)$$

This two-stage velocity update requires force evaluation only once per time step.

### Observables

| Observable | Definition |
|---|---|
| Temperature | $T = \frac{1}{N d}\sum_i m v_i^2$ (equipartition theorem, $k_B = 1$) |
| Kinetic Energy | $K = \frac{1}{2}\sum_i m v_i^2$ |
| Potential Energy | $U = \sum_{i<j} U_\text{LJ}(r_{ij})$ |
| Total Energy | $E = K + U$ |

---

## Code Structure

All simulation logic lives in two files:

| File | Role |
|---|---|
| `main.cpp` | Sets parameters, constructs `MolucularSystem`, calls `runSimulation()` and `save()` |
| `processing.h` | `MolucularSystem` class: integration, force calculation, energy tracking, I/O |

### `MolucularSystem` Class

Each `MolucularSystem` instance represents a complete simulation state for $N$ particles in a 2D periodic box.

#### Construction

```cpp
MolucularSystem(initialPositions, energyFunction, timeSteps, finalTime, boxSize)
```

- **Positions** are stored in a flat `std::vector<std::array<double, d>>` of size `timeSteps × numParticles`, accessed via `getPosition(t, i)` and `setPosition(t, i, pos)` using row-major indexing.
- **Velocities** and **accelerations** are single-frame arrays of size `numParticles`.
- All particles start at rest except particle 0, which receives a small initial velocity `{0.001, 0.001}` to break symmetry and seed energy flow.
- The dimensionality is compile-time configurable via `static constexpr int d = 2`.

#### Constants (Reduced Units)

| Constant | Value | Description |
|---|---|---|
| `d` | 2 | Spatial dimensions |
| `rc` | 2.5 σ | Lennard-Jones cutoff radius |
| `rc2` | 6.25 σ² | Precomputed $r_c^2$ to avoid square roots |

---

#### `calculateAccelerations(t)` — Force Calculation

Computes all pairwise Lennard-Jones forces for time step `t`:

```cpp
for (p1 = 0; p1 < N; p1++)
    for (p2 = p1+1; p2 < N; p2++)
        dx = p1.x - p2.x
        dx -= L * round(dx / L)     // minimum image convention
        r² = dx² + dy²
        if (r² > rc²) continue      // cutoff
        F = 24 * (2/r¹⁴ - 1/r⁸)    // computed as r × force / r²
        a[p1] += F;  a[p2] -= F     // Newton's third law
```

**Parallelism:** When `numParticles > 50`, the outer loop is parallelized with OpenMP using thread-local acceleration and potential energy accumulators, merged via `#pragma omp critical`. The schedule is `guided` to balance the triangular iteration pattern.

**Minimum image convention:** Periodic boundary conditions are enforced by shifting displacements to the nearest image: `dx -= L * round(dx / L)`.

---

#### `verletStep(t)` — Velocity Verlet Integration

Each call advances the system by one time step $\Delta t$:

1. **Half-kick:** Update velocities by half a time step using current accelerations
2. **Drift:** Update positions using the half-stepped velocities
3. **Wrap:** Apply periodic boundary conditions via `pos -= L * floor(pos / L)`
4. **Recalculate forces:** Call `calculateAccelerations(t)` at the new positions
5. **Half-kick:** Complete the velocity update with the new accelerations
6. **Energy bookkeeping:** Call `energyCalculations(t)`

Both velocity loops are parallelized with `#pragma omp parallel for` when `numParticles > 100`.

---

#### `energyCalculations(t)` — Energy Tracking & Injection

At each time step, the method:

1. Checks if the current step matches the next entry in `energyFunction` — if so, **rescales all velocities** by a factor $\lambda$:

$$\lambda = \sqrt{1 + \frac{2\,\Delta E_\text{desired}}{2K}}$$

This uniformly scales kinetic energy to inject or remove a precise amount of energy.

2. Records temperature, potential energy, kinetic energy, and total energy for postprocessing.

---

#### Energy Schedule: `buildEnergyFunction()`

The energy injection/extraction schedule is:

| Phase | Time Steps | Action |
|---|---|---|
| 0 – 5% | Settle | No energy change |
| 5 – 50% | Heating | Inject energy at a constant rate |
| 50 – 95% | Cooling | Remove energy at a constant rate |
| 95 – 100% | Settle | No energy change |

This drives the system through a full heating-cooling cycle, allowing observation of melting and crystallization behavior.

---

#### `runSimulation()` — Main Loop

```cpp
calculateAccelerations(0);      // initial forces
energyCalculations(0);          // initial energy bookkeeping
for (t = 1; t < timeSteps; t++)
    verletStep(t);              // advance the system
```

Progress is logged every 5% of the simulation.

---

#### Output Methods

| Method | Output |
|---|---|
| `saveResultsToNPZ()` | Compressed `.npz` with positions, velocities, accelerations, energies, temperatures, metadata |
| `saveEnergyToCSV()` | CSV with columns: TimeStep, Temperature, PE, KE, TE |
| `savePositionsToCSV()` | CSV with columns: TimeStep, ParticleIndex, X, Y |
| `save()` | Auto-creates `output/out_N/` (incrementing index) and calls all three savers |

---

## Visualization (`plotting.py`)

The Python pipeline performs two tasks:

### 1. Particle Animation (Parallel Rendering)

The animation is rendered in parallel using `multiprocessing`:

1. Frame indices are split across up to 128 workers
2. Each worker renders its segment to a temporary `.mp4` via `matplotlib.animation`
3. Segments are stitched with `ffmpeg -concat` into a single animation

This avoids the serial bottleneck of rendering thousands of frames on one core.

### 2. Energy Analysis Plots

Three-panel figure:

| Panel | Content |
|---|---|
| Top | Potential, kinetic, and total energy vs. time step |
| Middle | Temperature vs. time step |
| Bottom | Energy drift $\Delta E = E(t) - E(0)$ vs. time step |

---

## Results

### Energy Analysis

The energy plot shows the heating/cooling cycle: kinetic energy rises during the injection phase, plateaus, and falls during extraction. Total energy drift remains bounded, validating the Verlet integrator's symplectic properties.

### Particle Animation

The animation shows particles evolving from a regular grid through a gas-like disordered phase during heating, then re-ordering as energy is extracted — illustrating the connection between kinetic energy and temperature at the microscopic level.

---

## Sources of Error

| Source | Nature | Mitigation |
|---|---|---|
| Time-step discretization | Finite $\Delta t$ introduces integration error | Use smaller `finalTime / timeSteps`; Verlet is $O(\Delta t^2)$ per step |
| Cutoff truncation | Ignoring interactions beyond $r_c$ introduces a systematic energy offset | $r_c = 2.5\sigma$ captures $>98\%$ of the LJ well; could add tail corrections |
| Finite-size effects | Small $N$ or small box $L$ affects thermodynamic behavior | Increase particle count and box size |
| Velocity rescaling | Instantaneous rescaling is not thermostatting — not canonical ensemble | Acceptable for driven heating/cooling; use Nosé-Hoover for equilibrium studies |
| Initial symmetry | Grid start with zero velocities is artificial | Small perturbation on particle 0 breaks symmetry; long heating phase equilibrates |

**Computational complexity:** $\mathcal{O}(N^2 \times T)$ — all-pairs force calculation at each of $T$ time steps. For 400 particles and 300,000 steps this is $\sim 4.8 \times 10^{10}$ pair evaluations.

---

## Build & Run

### Prerequisites

- **C++17** compiler (`g++` or `clang++`)
- **OpenMP** (`brew install libomp` on macOS)
- **zlib** (for `.npz` output via `cnpy`)
- **Python 3** with `numpy` and `matplotlib`
- **ffmpeg** (for animation rendering)

### Build Targets

```bash
make release   # Optimized build (-O3, LTO, vectorization, march=native)
make unsafe    # Adds -ffast-math (may introduce minor FP drift)
make debug     # -O0, full warnings, OpenMP disabled
make profile-gen && ./bin/main && make profile-use  # Profile-guided optimization
```

### Run

```bash
./bin/main
```

Output is auto-saved to `output/out_N/` (incrementing index):

| File | Description |
|---|---|
| `results.npz` | Compressed NumPy archive with all simulation data |
| `energy_data.csv` | Time series of temperature and energies |
| `positions_data.csv` | Full particle trajectories |

#### NPZ Contents

| Array | Shape | Description |
|---|---|---|
| `positions` | `(timeSteps, N, 2)` | Particle positions at every time step |
| `velocities` | `(N, 2)` | Final velocities |
| `accelerations` | `(N, 2)` | Final accelerations |
| `temperatures` | `(timeSteps,)` | Instantaneous temperature |
| `potentialEnergies` | `(timeSteps,)` | Potential energy time series |
| `kineticEnergies` | `(timeSteps,)` | Kinetic energy time series |
| `totalEnergies` | `(timeSteps,)` | Total energy time series |
| `metadata` | `(4,)` | `[L, numParticles, timeSteps, finalTime]` |

### Visualize

```bash
python3 plotting.py
```

Produces `md_animation.mp4` and `energy_plot.png` in the latest `output/out_N/` directory.

### HPC (SLURM)

```bash
sbatch job.sh
```

The batch script builds in release mode, runs the simulation with 128 OpenMP threads, and generates visualizations automatically.

---

## Simulation Parameters

Configured in [main.cpp](main.cpp):

| Parameter | Default | Description |
|---|---|---|
| Grid | 20 × 20 | Initial particle grid (400 particles) |
| Spacing | 1.1 σ | Grid spacing with random jitter up to 0.7 σ |
| `timeSteps` | 300,000 | Number of integration steps |
| `finalTime` | 150.0 | Total simulation time (reduced units) |
| `boxSize` | 50.0 σ | Side length of periodic box |
| `rc` | 2.5 σ | Lennard-Jones cutoff radius |

---

## Key Techniques

| Technique | Purpose |
|---|---|
| Velocity Verlet integration | Time-reversible, symplectic $O(\Delta t^2)$ integrator |
| Lennard-Jones potential with cutoff | Realistic short-range interactions with $O(N^2)$ scaling |
| Minimum image convention | Correct nearest-image distances for periodic boundaries |
| Newton's third law ($\mathbf{F}_{ij} = -\mathbf{F}_{ji}$) | Halves the number of pair evaluations |
| Thread-local accumulators + `omp critical` reduce | Race-free parallel force summation |
| `guided` OpenMP schedule | Balances triangular pair-loop workload across threads |
| Velocity rescaling energy injection | Precisely controls energy input/output for driven simulations |
| Flat `std::array<double, d>` storage | Cache-friendly, compile-time dimensionality |
| `cnpy` NPZ output | Compact binary format directly loadable by NumPy |
| Parallel animation rendering | Multiprocessing + ffmpeg concat avoids serial matplotlib bottleneck |
| Profile-guided optimization (PGO) | Compiler uses runtime data for branch prediction and inlining |

---

## Project Structure

```
Project 8: Molicular Dynamics/
├── main.cpp        # Entry point: configures and launches the simulation
├── processing.h    # MolucularSystem class: integration, forces, energy, I/O
├── Makefile        # Multi-target build: debug, release, unsafe, profile-guided
├── plotting.py     # Python visualization: parallel animation + energy plots
├── job.sh          # SLURM batch script (BYU Supercomputer — 128 CPUs, 1 hr)
├── bin/            # Compiled executable
├── slurm_out/      # SLURM output logs from HPC runs
└── output/
    ├── Test Out/           # Early test run
    ├── out_0/ – out_9/     # Simulation runs (auto-incrementing)
    │   ├── results.npz            # Compressed simulation data (NumPy)
    │   ├── energy_data.csv        # Energy & temperature time series (CSV)
    │   ├── positions_data.csv     # Full particle trajectories (CSV)
    │   ├── md_animation.mp4       # Particle trajectory animation
    │   └── energy_plot.png        # Energy analysis figure
    └── ...
```

---

*Nels Buhrley — Computational Physics, 2026*
