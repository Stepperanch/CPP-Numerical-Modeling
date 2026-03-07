# Computational Physics Portfolio

**Nels Buhrley**
*Physics Student — Brigham Young University–Idaho*

---

## Table of Contents

- [About](#about)
  - [Core Competencies](#core-competencies)
  - [The Fulton Supercomputer at BYU](#the-fulton-supercomputer-at-byu)
- [Academic Projects](#academic-projects)
  - [Progression](#progression)
- [Personal Projects](#personal-projects)
- [Selected Results](#selected-results)
- [Quick Start](#quick-start)
  - [Build Toolchain](#build-toolchain)
  - [Prerequisites](#prerequisites)
- [Repository Structure](#repository-structure)

---

## About

This repository is a collection of **nine numerical simulation and computational mathematics projects** developed during my physics coursework and independent study. The work spans classical mechanics, electrostatics, statistical mechanics, and number theory — all implemented in **C++17** with **Python 3** visualization pipelines.

### Core Competencies

| Area | Details |
|---|---|
| **Languages** | C++17, Python 3 |
| **Numerical Methods** | Runge-Kutta (RK4), Euler-Cromer, Störmer-Verlet, Monte Carlo (Metropolis), Finite Differences, Successive Over-Relaxation, FFT spectral analysis |
| **Parallelism** | OpenMP multi-threading (Projects 4–7, Personal Project 2) |
| **High-Performance Computing** | BYU Supercomputer — SLURM batch scheduling with up to 128 CPU cores for intensive simulations (Projects 4, 6, 7) |
| **I/O & Visualization** | CSV, NPZ (via cnpy/zlib), Matplotlib (3D surfaces, contour maps, animations, phase-space portraits) |
| **Build Systems** | GNU Make with multi-target builds (debug, release, profile-guided optimization) |

### The Fulton Supercomputer at BYU

The **Fulton Supercomputer** (managed by the BYU Office of Research Computing) is a HPC cluster providing the processing backbone for my numerical modeling and simulation work. It manages over **35,000 CPU cores** and **360+ GPUs**, supported by a **6 PB** parallel filesystem.

<p align="center">
  <img src="assets/me-at-byuorc.jpeg" alt="Me At Fulton Supercomputer" width="45%"/>
  <br>
  <em>Figure 1: Me at the BYU HPC cluster Febuary 2026.</em>
</p>

---

## Architecture & Resources (2026 Specs)

| Component | Specification |
| :--- | :--- |
| **Compute Nodes** | Heterogeneous: **AMD EPYC 7763** (128 cores/node), **Intel Xeon Platinum 8568Y+** (96 cores/node)... |
| **High-Memory** | Specialized nodes providing up to **2 TB of DDR5 RAM** for memory-intensive simulations |
| **GPU Acceleration** | **NVIDIA H200 (141GB)**, **L40S (48GB)**, and **A100 (80GB)** units |
| **Interconnect** | **100 Gb/s InfiniBand** and RoCE v2 low-latency networking |
| **Storage** | **6 PB** parallel filesystem via `/fslhome` and local **NVMe scratch** space |

---

## Workflow & Scheduling

### Job Management
I utilize **SLURM (Simple Linux Utility for Resource Management)** to orchestrate simulations. This involves writing batch scripts that request specific hardware constraints to optimize performance, such as:
* `--constraint=avx512` for vector instructions.
* `--qos=standby` for extended 7-day simulation windows.

### Environment & Compilation
Development is performed on **RHEL 9.4** login nodes. I manage software dependencies via the `module load` system, typically involving:
* **GCC/G++** for core simulation logic.
* **OpenMPI** for distributed memory parallelism.
* **Python 3/ffmpeg** for visiulation

---

## Physics Implementation
On this cluster, I implement numerical solvers for complex potentials where the computation scales as $O(N^2)$. For a system of $N$ particles, the potential $V$ is calculated as:

$$V = \sum_{i < j} \frac{q_i q_j}{4\pi\epsilon_0 |\mathbf{r}_i - \mathbf{r}_j|}$$

By utilizing **OpenMP** for multi-threading and **MPI** for node-to-node communication, I can distribute these calculations, significantly reducing the wall-time required for high-resolution datasets.

```cpp
// Example: Basic OpenMP Parallelization for Force Calculation
#pragma omp parallel for reduction(+:total_energy)
for (int i = 0; i < N; ++i) {
    for (int j = i + 1; j < N; ++j) {
        total_energy += calculate_interaction(particles[i], particles[j]);
    }
}
```

**Simulation Deployment (This Portfolio):**
| Project | Configuration | CPUs | Wall Time | Use Case |
|---|---|---|---|---|
| Project 4 (SOR) | CPU-only, `#threads=16` | 16 | 30–45 min | Strong-scaling study of iterative PDE solver |
| Project 6 (Diffusion) | CPU-only, `#threads=32` | 32 | 2-3 min | Parallelizing independent particle trajectories |
| Project 7 (Ising) | CPU-only, `#threads=128` | 128 | 10–45 min | Large parameter-space sweep with checkerboard MCMC |
| Project 8 (Molecular Dynamics) | CPU-only, `#threads=8` | 8 | 10 min–2 hrs | Thread-local force accumulation (race condition mitigation) |

**Key Advantages for This Work:**
- **Scalability Testing:** Weak and strong scaling studies for OpenMP efficiency (Projects 4, 7)
- **Parameter Sweeps:** Multi-dimensional search spaces (Project 7: 2D temperature × field grid)
- **Long-Running Simulations:** Statistical ensembles (Project 6: millions of particle trajectories)
- **Reproducibility:** Identical hardware across multiple runs for benchmarking and verification

---

## Academic Projects

| # | Project | Physical System | Numerical Method | OpenMP | HPC |
|---|---|---|---|---|---|
| 1 | [Realistic Projectile Motion](Project%201%3A%20realistic%20projectile%20motion/) | 3D ballistics with drag, spin (Magnus), wind | 4th-order Runge-Kutta | — | — |
| 2 | [Driven Damped Oscillations](Project%202%3A%20driven%20damped%20oscillations/) | Nonlinear pendulum → periodic & chaotic regimes | RK4 + Euler-Cromer; Poincaré sections | — | — |
| 3 | [Celestial Dynamics](Project%203%3A%20Celestial%20Dynamics/) | N-body gravitational orbits (full Solar System) | RK4, Euler-Cromer, Störmer-Verlet | — | — |
| 4 | [Overrelaxation](Project%204%3A%20Overrelaxation/) | 3D Laplace's equation (electrostatics) | Red-Black SOR with optimal $\omega$ | ✓  | ✓ |
| 5 | [Oscillations on a String](Project%205%3A%20Occilations%20on%20a%20string/) | Damped stiff wave equation + spectral analysis | Finite differences (2nd + 4th order) + KissFFT | ✓ parallel spatial | — |
| 6 | [Diffusion](Project%206%3A%20Diffusion/) | 3D Brownian random-walk ensemble | Monte Carlo with reflective BCs | ✓ parallel particles | ✓ |
| 7 | [The Ising Model](Project%207%3A%20The%20Ising%20Model/) | 3D ferromagnetic phase transition | Metropolis MCMC, checkerboard sweep | ✓ multifactor parallel | ✓ 128 CPUs |
| 8 | [Molecular Dynamics](Project%208%3A%20Molicular%20Dynamics/) | 2D Lennard-Jones fluid (400 particles) | Velocity Verlet, O(N²) pair forces | ✓ thread-local accumulators | ✓ 8 CPUs |

### Progression

The projects follow a deliberate arc of increasing computational sophistication:

- **Projects 1–3** build fluency with ODE integration (RK4, symplectic methods) and interactive simulations
- **Project 4** introduces PDE solving, iterative methods, and OpenMP parallelism
- **Projects 5–6** combine PDE/stochastic methods with spectral analysis and 3D particle tracking
- **Project 7** synthesizes everything: statistical physics, Monte Carlo methods, precomputed lookup tables, multi-dimensional parameter sweeps, and full HPC deployment
- **Project 8 (Capstone)** tackles the hardest parallelization challenge — an $O(N^2)$ N-body problem where Newton's third law optimizations create race conditions, resolved via thread-local accumulators and guided scheduling

---

## Personal Projects

| # | Project | Domain | Key Challenge | Parallelism |
|---|---|---|---|---|
| 1 | [Collatz Conjecture (3n+1)](Personal%20Project%201%3A%203n%2B1/) | Number theory | Exhaustive sequence analysis for $n \leq 10^6$; recursive max-value chaining | — |
| 2 | [Euler's Idoneal Numbers](Personal%20Project%202%3A%20Idelic%20Numbers/) | Number theory | Sieve over triple loop $a < b < c$ up to $5 \times 10^7$; thread-safe monotonic writes | ✓ OpenMP `dynamic` |

---

## Selected Results

<p align="center">
  <img src="Project%207%3A%20The%20Ising%20Model/output/Out_1/magnetization_3d_surface_angle3.png" alt="Ising Model — 3D magnetization surface" width="48%"/>
  <img src="Project%207%3A%20The%20Ising%20Model/output/Out_1/magnetization_contour.png" alt="Ising Model — contour map" width="48%"/>
</p>

Project 7: Magnetization surface and contour map of the 3D Ising model, revealing the ferromagnetic phase transition at $T_c \approx 4.51\,J/k_B$.

<p align="center">
  <img src="Project%203%3A%20Celestial%20Dynamics/Output/celestial_analysis_9_3d.png" alt="Celestial Dynamics — 3D orbits" width="48%"/>
  <img src="Project%205%3A%20Occilations%20on%20a%20string/mean_power_spectrum.png" alt="String oscillations — power spectrum" width="48%"/>
</p>

Left: N-body orbital dynamics of the Solar System. <br> Right: Mean power spectrum of transverse string oscillations showing normal-mode peaks.

<p align="center">
  <img src="Project%204%3A%20Overrelaxation/output/potential_3D_center_slice.png" alt="Electrostatic potential slice" width="48%"/>
  <img src="Project%206%3A%20Diffusion/output/fixed_Mean_Squared_Distance_plot.png" alt="Diffusion MSD" width="48%"/>
</p>

Left: 2D slice through a 3D electrostatic potential field (SOR solver, $N=1000^3$). <br> Right: Mean squared displacement confirming Einstein's diffusion law $\langle r^2 \rangle = 6Dt$.

---

## Quick Start

Every project follows the same workflow:

```bash
# 1. Navigate to a project
cd "Project 7: The Ising Model"

# 2. Build
make release

# 3. Run
./bin/main

# 4. Visualize
python3 plotting.py
```

### Build Toolchain

| Tool | Version | Notes |
|---|---|---|
| **Compiler** | `clang++` / `g++` (C++17) | Platform-detecting Makefiles |
| **OpenMP** | Homebrew `libomp` (macOS) / native (Linux) | Projects 4–7 |
| **zlib** | System | For `.npz` output via `cnpy` |
| **Python** | 3.x | `numpy`, `matplotlib`, `pandas` |
| **SLURM** | BYU Supercomputer | Batch scheduling for HPC runs |

### Prerequisites

```bash
# macOS
brew install libomp
pip3 install numpy matplotlib pandas

# Linux (e.g., BYU Supercomputer)
module load gcc python   # or equivalent
pip3 install numpy matplotlib pandas
```

---

## Repository Structure

```
CPP_Workspace/
├── README.md                          # ← This file (Portfolio Hub)
│
├── Project 1: realistic projectile motion/    # RK4 ballistics with drag & Magnus
├── Project 2: driven damped oscillations/     # Nonlinear pendulum & chaos
├── Project 3: Celestial Dynamics/             # N-body gravitational simulation
├── Project 4: Overrelaxation/                 # 3D Laplace solver (SOR + OpenMP)
├── Project 5: Occilations on a string/        # Wave equation FD + FFT spectral
├── Project 6: Diffusion/                      # 3D random-walk Monte Carlo
├── Project 7: The Ising Model/                # 3D Metropolis MCMC (HPC)
│
├── Personal Project 1: 3n+1/                 # Collatz conjecture exploration
├── Personal Project 2: Idelic Numbers/        # Euler's idoneal number sieve
│
├── include/                                   # Shared libraries
│   ├── cnpy.h                                 # NPZ I/O library
│   └── vector3d.h                             # 3D vector utilities
├── src/
│   └── cnpy.cpp                               # cnpy implementation
│
├── reference/                                 # Study notes & cheatsheets
│   ├── CPP_BASICS.md
│   ├── FUNCTIONS_REFERENCE.md
│   ├── NUMERICAL_PHYSICS.md
│   ├── OPTIMIZATION_TIPS.md
│   └── SLURM_FORMATING.md
│
└── Template Makefile                          # Reusable build template
```

---

*Nels Buhrley — BYU-Idaho Physics, 2025–2026*
