#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#define _USE_MATH_DEFINES

#include <omp.h>

#include "cnpy.h"

#ifndef PROCESSING_H
#define PROCESSING_H

/**
 *  File Header
 *   Author: Nels Buhrley
 *   Date: 2026-17-02
 *   Description:
 * This file contains the implementation of the Material and Simulation classes for simulating the 3D Ising model using the Metropolis algorithm.
 *  The Material class represents the physical system, including the lattice of spins, temperature, magnetic field, and methods for initializing
 *  and updating the system. The Simulation class manages the overall simulation process, including running the simulations across a range of
 *  temperatures and magnetic fields, analyzing the results to find critical temperatures and beta exponents, and saving the results to files.
 *  The code is designed to be efficient and scalable, utilizing OpenMP for parallelization and precomputing energy tables for faster spin
 *  updates. The results are stored in numpy-compatible .npy files for easy analysis and visualization.
 *
 */

class Material {
   public:
    int n;              // Size of the lattice (n x n x n)
    int actual_n;       // Actual size of the lattice excluding boundaries
    int N;              // Total number of spins in the lattice (actual_n^3)
    float temperature;  // Temperature of the system
    float h;            // Total external magnetic field of the system
    int numIterations;  // Number of iterations to run the simulation

    float averageMagnetization;         // Running average magnetization for current temperature and magnetic field
    float averageMagnetizationSquared;  // Running average of magnetization squared for current temperature and magnetic field
    float averageAbsMagnetization;      // Running average of absolute magnetization for current temperature and magnetic field

    float magneticSusceptibility;  // Magnetic susceptibility for current temperature and magnetic field

    int currentTotalMagnetization;  // Current total magnetization of the lattice (used for incremental updates)

    std::vector<int8_t> spins;  // 3D array: [x][y][z]

    float deltaE_table[2][7];  // Precomputed energy changes for spin flips
    float exp_table[2][7];     // Precomputed exp(-ΔE/T) values

    std::uniform_real_distribution<float> distribution;  // For random number generation
    std::mt19937 gen;                                    // Mersenne Twister RNG
    uint32_t seed;                                       // Seed for RNG

   public:
    Material(int n, float temperature, float magnetization, int numIterations, uint32_t seed)
        : n(n + 2),
          actual_n(n),
          N(actual_n * actual_n * actual_n),
          temperature(temperature),
          h(magnetization),
          numIterations(numIterations),
          seed(seed) {
        establishRNG();
        initializeSpinsRandomly();
        precalculateEnergyTables();
    }

    Material(int n, float temperature, float magnetization, int numIterations, int8_t initialSpinValue, uint32_t seed)
        : n(n + 2),
          actual_n(n),
          N(actual_n * actual_n * actual_n),
          temperature(temperature),
          h(magnetization),
          numIterations(numIterations),
          seed(seed) {
        establishRNG();
        initializeSpinsUniformly(initialSpinValue);
        precalculateEnergyTables();
    }

    // Accessor and mutator for spins at specific lattice coordinates (x, y, z)
    inline int8_t getSpin(int x, int y, int z) {
        return spins[x * n * n + y * n + z];
    }

    inline void setSpin(int x, int y, int z, int8_t value) {
        spins[x * n * n + y * n + z] = value;
    }

    /**
     * @brief Initializes the spins of the lattice randomly to either +1 or -1.
     * Each spin is assigned a value based on a uniform distribution, ensuring an equal probability of being +1 or -1.
     * The method iterates through the inner lattice (excluding the boundary layers)
     */
    void initializeSpinsRandomly() {
        currentTotalMagnetization = 0;
        spins.resize(n * n * n);
        for (int x = 1; x < n - 1; x++) {
            for (int y = 1; y < n - 1; y++) {
                for (int z = 1; z < n - 1; z++) {
                    setSpin(x, y, z, (distribution(gen) < 0.5 ? -1 : 1));  // Randomly +1 or -1
                    currentTotalMagnetization += getSpin(x, y, z);
                }
            }
        }
    }

    /**
     * @brief Initializes the spins of the lattice to a uniform value, either +1 or -1.
     * This method fills the entire inner lattice (excluding the boundary layers) with the specified spin value.
     * The spin value must be either +1 or -1; otherwise, an exception is thrown.
     *
     * @param spinValue The uniform spin value to initialize the lattice with (+1 or -1).
     */
    void initializeSpinsUniformly(int8_t spinValue) {
        if (spinValue != 1 && spinValue != -1) {
            throw std::invalid_argument("Spin value must be +1 or -1");
        }
        spins.resize(n * n * n, spinValue);
        currentTotalMagnetization = spinValue * N;
    }

    /**
     * @brief Precalculates the energy tables for efficient simulation.
     * This method precomputes the energy differences and corresponding exponential factors for all possible spin configurations.
     * The precomputed values are used during the simulation to avoid redundant calculations.
     */
    void precalculateEnergyTables() {
        int8_t neighborValues[7] = {-6, -4, -2, 0, 2, 4, 6};
        int8_t spinValues[2] = {-1, 1};

        // Precompute exp(-ΔE/T) for all possible ΔE values and spin states
        for (int s = 0; s < 2; s++) {
            for (int i = 0; i < 7; i++) {
                float deltaE = 2 * spinValues[s] * (neighborValues[i] + h);
                deltaE_table[s][i] = deltaE;
                exp_table[s][i] = exp(-deltaE / temperature);
            }
        }
    }

    /**
     * @brief Establishes the random number generator for the simulation in a thread-safe manner.
     * This method initializes the Mersenne Twister RNG with a unique seed for each thread
     */
    void establishRNG() {
        // Thread-safe random number generator setup
        gen = std::mt19937(seed);
        distribution = std::uniform_real_distribution<float>(0.0, 1.0);
    }

    /**
     * @brief Attempts to flip the spin at lattice site (x, y, z) via the Metropolis criterion.
     *
     * Algorithm:
     *   1. Compute the sum of the six nearest-neighbour spins (each ±1).
     *      The sum lies in the discrete set {-6, -4, -2, 0, 2, 4, 6}.
     *   2. Map that sum to a table index in [0, 6] by dividing by 2 and adding 3.
     *   3. Map the current spin (−1 or +1) to a table index (0 or 1).
     *   4. Look up the precomputed ΔE and exp(−ΔE/T) for this configuration.
     *   5. Accept the flip deterministically if ΔE ≤ 0, or probabilistically
     *      with probability exp(−ΔE/T) (standard Metropolis acceptance rule).
     *   6. If accepted, invert the spin and update the running magnetisation tally.
     *
     * Using a lookup table (precomputed in precalculateEnergyTables) avoids calling
     * std::exp() inside the innermost loop, which is a significant speedup.
     */
    void flipSpin(int x, int y, int z) {
        // Sum of all six nearest-neighbour spin values; lies in {-6, -4, -2, 0, 2, 4, 6}
        int neighborSum = getSpin(x + 1, y, z) + getSpin(x - 1, y, z)
                        + getSpin(x, y + 1, z) + getSpin(x, y - 1, z)
                        + getSpin(x, y, z + 1) + getSpin(x, y, z - 1);

        // Convert neighbour sum to a 0-based table index:
        //   sum / 2 shifts the step size from 2 → 1; +3 maps the range [-3,3] → [0,6]
        uint8_t neighborstate = neighborSum / 2 + 3;

        // Convert spin value {-1, +1} to a 0-based table index {0, 1}:
        //   (spin + 1) / 2  maps  -1 → 0,  +1 → 1
        uint8_t spinState = (getSpin(x, y, z) + 1) / 2;

        // Metropolis acceptance:  always accept if ΔE ≤ 0 (energy decreases or stays same);
        // otherwise accept with Boltzmann probability exp(-ΔE/T).
        if (deltaE_table[spinState][neighborstate] <= 0
                || distribution(gen) < exp_table[spinState][neighborstate]) {
            setSpin(x, y, z, -getSpin(x, y, z));                // Invert the spin
            currentTotalMagnetization += 2 * getSpin(x, y, z);  // ΔM = ±2 per flip
        }
    }

    /**
     * @brief Performs one iteration of the Metropolis algorithm across the entire lattice.
     * The method consists of two main parts:
     * 1. Updating the boundary conditions: The outer layers of the lattice are updated to mirror the inner lattice, ensuring periodic boundary
     * conditions.
     * 2. Iterating through the inner lattice: The method iterates through the inner lattice (excluding the boundary layers) in a Black and Red
     * pattern and attempts to flip spins based on their coordinates.
     */
    void iteration() {
        int x, y, z;
        for (x = 0; x < n; x++) {
            for (y = 0; y < n; y++) {
                setSpin(x, y, 0, getSpin(x, y, n - 2));
                setSpin(x, y, n - 1, getSpin(x, y, 1));
            }
        }
        for (x = 0; x < n; x++) {
            for (z = 0; z < n; z++) {
                setSpin(x, 0, z, getSpin(x, n - 2, z));
                setSpin(x, n - 1, z, getSpin(x, 1, z));
            }
        }
        for (y = 0; y < n; y++) {
            for (z = 0; z < n; z++) {
                setSpin(0, y, z, getSpin(n - 2, y, z));
                setSpin(n - 1, y, z, getSpin(1, y, z));
            }
        }
        // ── Checkerboard (Red-Black) Metropolis Sweep ────────────────────────────
        // The lattice is partitioned into two interleaved sub-lattices ("black" and
        // "red") such that no two sites in the same sub-lattice are nearest neighbours.
        // This lets all sites within one sub-lattice be updated independently in a
        // single pass, which is critical for correct OpenMP parallelisation: if we
        // updated arbitrary sites simultaneously, two threads might read/write
        // adjacent spins and produce a data race.
        //
        // The parity of a site is (x + y + z) % 2.  By fixing x and y in the outer
        // loops and stepping z by 2 starting at the appropriate parity offset we
        // visit exactly one sub-lattice per inner loop.

        // Pass 1: "black" sites – sites where (x + y + z) is even
        for (x = 1; x < n - 1; x++) {
            for (y = 1; y < n - 1; y++) {
                // Starting z: 1 if (x+y) is even (so x+y+1 is odd → skip); compensate
                for (z = (x + y) % 2 + 1; z < n - 1; z += 2) {
                    flipSpin(x, y, z);
                }
            }
        }
        // Pass 2: "red" sites – sites where (x + y + z) is odd
        for (x = 1; x < n - 1; x++) {
            for (y = 1; y < n - 1; y++) {
                for (z = (x + y + 1) % 2 + 1; z < n - 1; z += 2) {
                    flipSpin(x, y, z);
                }
            }
        }
    }

    /**
     * @brief Runs the full simulation for the specified number of iterations.
     * This method repeatedly calls the iteration() method, which performs one iteration of the Metropolis
     * algorithm across the entire lattice, for the total number of iterations specified in the constructor.
     * The method allows the simulation to evolve over time, enabling the system to reach equilibrium and
     */
    /**
     * @brief Runs the complete Metropolis simulation and records ensemble averages.
     *
     * Two phases:
     *   Warmup  – 5*n sweeps are discarded to allow the lattice to relax from
     *             its (possibly far-from-equilibrium) initial configuration
     *             toward the true equilibrium state at the given T and h.
     *             Using 5*(n+2) sweeps gives each spin ≈5 update attempts on
     *             average per equilibration sweep.
     *
     *   Sampling – numIterations sweeps are performed and the per-sweep
     *              magnetisation m = M/N is accumulated.  Three averages are
     *              computed from these samples:
     *                <m>    – average magnetisation (cancels for a symmetric system)
     *                <|m|>  – average absolute magnetisation (order parameter)
     *                <m²>   – needed to calculate magnetic susceptibility χ
     */
    void runSimulation() {
        float sum_magnetization         = 0.0;
        float sum_magnetization_squared = 0.0;
        float sum_abs_magnetization     = 0.0;

        // ── Warmup Phase ─────────────────────────────────────────────────────
        // Discard the first 5*n sweeps to thermalise the system.
        // (n here is n+2 counting the ghost layers, so effectively ≈5*actual_n.)
        for (int i = 0; i < 5 * n; i++) {
            iteration();
        }

        // ── Sampling Phase ───────────────────────────────────────────────────
        // Accumulate statistics over numIterations production sweeps.
        for (int i = 0; i < numIterations; i++) {
            iteration();
            // Per-spin magnetisation m ∈ [−1, 1]
            float currentMagnetization = (float)currentTotalMagnetization / N;
            sum_magnetization         += currentMagnetization;
            sum_magnetization_squared += currentMagnetization * currentMagnetization;
            sum_abs_magnetization     += std::abs(currentMagnetization);
        }

        // Finalise time-averages by dividing by the number of production sweeps
        averageMagnetization        = sum_magnetization         / numIterations;
        averageAbsMagnetization     = sum_abs_magnetization     / numIterations;
        averageMagnetizationSquared = sum_magnetization_squared / numIterations;
    }

    /**
     * @brief Computes the magnetic susceptibility χ from the fluctuation-dissipation theorem.
     *
     * The susceptibility is related to magnetisation fluctuations by:
     *
     *   χ = N / T * ( <m²> − <|m|>² )
     *
     * where N = actual_n³ is the total number of spins, T is the temperature (in
     * units of J/kB), m = M/N is the per-spin magnetisation, and the angle brackets
     * denote ensemble (time) averages.  Near the critical temperature Tc, χ diverges
     * and its peak position is used to estimate Tc in findCriticalTemperatureAndCalculateBeta().
     *
     * Note: we use <|m|>² rather than <m>² because for a finite lattice the system
     * can spontaneously flip between the two degenerate ground states (m = ±m₀),
     * making <m> ≈ 0 even below Tc.  <|m|> correctly captures the magnitude.
     */
    void MagneticSusceptibility() {
        // χ = N/T * Var(|m|)  where Var(|m|) = <m²> − <|m|>²
        magneticSusceptibility =
            actual_n * actual_n * actual_n
            * (averageMagnetizationSquared - averageAbsMagnetization * averageAbsMagnetization)
            / temperature;
    }
};

class Simulation {
   public:
    std::vector<std::vector<double>> avg_magnetizations;
    std::vector<std::vector<double>> magnetic_susceptibilities;
    std::vector<float> temperatures;
    std::vector<float> magnetic_fields;

    std::vector<float> critical_temperatures;
    std::vector<int> critical_indices;
    std::vector<float> beta_exponents;

    float averageBetaExponent_h0;  // Average beta exponent at zero magnetic field
    float averageCriticalTemperature_h0;  // Average critical temperature at zero magnetic field

    int n;
    int iterations;
    float hMin;
    float hMax;
    int numHSteps;
    float hStep;
    float tempMin;
    float tempMax;
    int numTempSteps;
    float tempStep;

    Simulation(int n, int iterations, float hMin, float hMax, int numHSteps, float tempMin, float tempMax, int numTempSteps) {
        this->n = n;
        this->iterations = iterations;
        this->hMin = hMin;
        this->hMax = hMax;
        this->numHSteps = numHSteps;
        this->tempMin = tempMin;
        this->tempMax = tempMax;
        this->numTempSteps = numTempSteps;
        hStep = (hMax - hMin) / (numHSteps - 1);
        tempStep = (tempMax - tempMin) / (numTempSteps - 1);

        // numHSteps = numHSteps + 1;

        temperatures.resize(numTempSteps);
        magnetic_fields.resize(numHSteps);
        magnetic_fields[0] = hMin;

        for (int i = 1; i < numHSteps; i++) {
            // if (magnetic_fields[i-1] > 0 && magnetic_fields[i] < 0) {
            //     magnetic_fields[i] = 0.0f;  // Ensure we include zero field
            // } else {
            //     magnetic_fields[i] = hMin + i * hStep;
            // }

            magnetic_fields[i] = hMin + i * hStep;
        }

        // ── Zero-Field Snap ───────────────────────────────────────────────────
        // If the field range straddles zero (hMin < 0 < hMax), we snap the grid
        // point nearest to h = 0 to exactly 0.0.  This ensures that the
        // spontaneous-magnetisation curve (h = 0) is always present in the output,
        // which is required for the critical-exponent β analysis.
        // The nearest index is round(-hMin / hStep) clamped to [0, numHSteps-1].
        if (hMin < 0 && hMax > 0) {
            int zeroIndex = static_cast<int>(-hMin / hStep);
            magnetic_fields[zeroIndex] = 0.0f;
            std::cout << "Adjusted magnetic field at index " << zeroIndex
                      << " to include zero field: " << magnetic_fields[zeroIndex] << std::endl;
        }

        for (int i = 0; i < numTempSteps; i++) {
            temperatures[i] = tempMin + i * tempStep;
        }

        avg_magnetizations.resize(numHSteps, std::vector<double>(numTempSteps));
        magnetic_susceptibilities.resize(numHSteps, std::vector<double>(numTempSteps));
        critical_temperatures.resize(numHSteps);
        critical_indices.resize(numHSteps);
        beta_exponents.resize(numHSteps);
    }

    /**
     * @brief Performs the full 2D parameter sweep over (temperature, magnetic field) pairs.
     *
     * Step 1 – Seed generation (serial):
     *   Each (T, h) cell gets a unique, independently drawn 32-bit seed so that
     *   parallel threads produce statistically independent random sequences.  Seeds
     *   are generated from a hardware-seeded Mersenne Twister before the parallel
     *   region to avoid any thread-safety issues with std::random_device.
     *
     * Step 2 – Parallel sweep (OpenMP):
     *   collapse(2) merges the two nested loops into a single flat loop of
     *   numTempSteps * numHSteps iterations, giving OpenMP more work units to
     *   distribute across threads.  schedule(dynamic) is used because the work
     *   per cell can vary (the system thermalises faster at high T).
     *
     *   Each thread constructs its own Material object (stack-allocated, no shared
     *   mutable state between threads), runs the simulation, then writes the result
     *   into the 2-D result arrays.  The result arrays are indexed [h_index][T_index]
     *   so that each cell is written by exactly one thread (no race conditions).
     */
    void runSimulation() {
        // ── Step 1: Pre-generate unique seeds for every (T, h) pair ─────────
        // Using a single master RNG here (serial) keeps seed generation deterministic
        // and avoids std::random_device overhead inside the parallel loop.
        std::random_device rd;
        std::mt19937 master_gen(rd());
        std::uniform_int_distribution<uint32_t> seed_dist;
        // thread_seeds[h_index][T_index] – matches the indexing of the result arrays
        std::vector<std::vector<uint32_t>> thread_seeds(numHSteps, std::vector<uint32_t>(numTempSteps));

        for (int i = 0; i < numHSteps; i++) {
            for (int j = 0; j < numTempSteps; j++) {
                thread_seeds[i][j] = seed_dist(master_gen);
            }
        }

        // ── Step 2: Parallel (T, h) sweep ───────────────────────────────────
        // Each Material is fully self-contained, so threads share no mutable data.
        // The initial spin configuration is all-up (+1) so every run starts from
        // the ordered (ferromagnetic) state, which helps convergence below Tc.
#pragma omp parallel for collapse(2) schedule(dynamic)
        for (int i = 0; i < numTempSteps; i++) {
            for (int j = 0; j < numHSteps; j++) {
                // Construct and thermalise the lattice at this (T, h) point
                Material material(n, temperatures[i], magnetic_fields[j], iterations,
                                  /*initialSpinValue=*/1, thread_seeds[j][i]);
                material.runSimulation();

                // Store per-spin average magnetisation <|m|>
                avg_magnetizations[j][i] = material.averageMagnetization;

                // Compute and store susceptibility χ from fluctuation-dissipation theorem
                material.MagneticSusceptibility();
                magnetic_susceptibilities[j][i] = material.magneticSusceptibility;
            }
        }
    }

    /**
     * @brief Locates the critical temperature Tc for each magnetic field value and
     *        estimates the critical exponent β via a log-log regression.
     *
     * Critical Temperature Detection
     * --------------------------------
     * Near the phase transition the magnetic susceptibility χ diverges.  On a
     * finite lattice the peak of χ(T) is a reliable proxy for Tc.  For each field
     * value h we scan temperatures[i] and record the index of the χ maximum.
     *
     * Critical Exponent β
     * --------------------
     * Below Tc the order parameter (spontaneous magnetisation) vanishes as a power law:
     *
     *   |m(T)| ~ (Tc − T)^β   as  T → Tc⁻
     *
     * Taking logarithms:  log|m| = β·log(Tc − T) + const
     *
     * We perform ordinary least-squares linear regression on (log(Tc−T), log|m|)
     * for data points in a window just below Tc.  The slope of the fit gives β.
     * The exact value for the 3D Ising model is β ≈ 0.326.
     *
     * Data points are excluded if:
     *   • |m| < 0.01  (magnetisation is essentially zero → log would be -∞)
     *   • Tc − T ≤ 0  (at or above Tc where the power law does not apply)
     */
    void findCriticalTemperatureAndCalculateBeta() {
        // Process each magnetic-field column independently (embarrassingly parallel)
#pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < numHSteps; j++) {

            // ── Step 1: Find Tc as the temperature of maximum susceptibility ─────
            double maxSusceptibility = -1.0;
            int criticalTempIndex    = -1;
            for (int i = 0; i < numTempSteps; i++) {
                if (magnetic_susceptibilities[j][i] > maxSusceptibility) {
                    maxSusceptibility  = magnetic_susceptibilities[j][i];
                    criticalTempIndex  = i;
                }
            }
            critical_temperatures[j] = temperatures[criticalTempIndex];
            critical_indices[j]      = criticalTempIndex;

            // ── Step 2: Collect data points in the critical scaling window ───────
            // We look at up to 40 temperature points immediately below Tc.
            // Points are included only when both |m| and (Tc − T) are positive
            // so that their logarithms are well-defined.
            std::vector<double> magnetizationNearTc;
            std::vector<double> tempDiffs;

            int startIndex = std::max(0, critical_indices[j] - 40);  // Window lower bound
            int endIndex   = critical_indices[j];                     // Up to (but not including) Tc

            for (int i = startIndex; i < endIndex; i++) {
                double absM = std::abs(avg_magnetizations[j][i]);
                double dT   = critical_temperatures[j] - temperatures[i];
                if (absM > 0.01 && dT > 0.0) {
                    magnetizationNearTc.push_back(absM);
                    tempDiffs.push_back(dT);
                }
            }

            // Need at least 2 points to define a line
            if (magnetizationNearTc.size() < 2) {
                beta_exponents[j] = std::numeric_limits<float>::quiet_NaN();
                continue;
            }

            // ── Step 3: Log-log OLS regression to extract β ──────────────────
            // We minimise Σ(log|m| − β·log(Tc−T) − c)² with respect to β and c.
            // The closed-form OLS slope is:
            //   β = [n·Σ(logT·logM) − Σ(logT)·Σ(logM)] / [n·Σ(logT²) − (Σ logT)²]
            double sumLogM  = 0.0, sumLogT  = 0.0;
            double sumLogT2 = 0.0, sumLogMT = 0.0;
            for (size_t k = 0; k < magnetizationNearTc.size(); k++) {
                double logM = log(std::abs(magnetizationNearTc[k]));
                double logT = log(tempDiffs[k]);
                sumLogM  += logM;
                sumLogT  += logT;
                sumLogT2 += logT * logT;
                sumLogMT += logM * logT;
            }

            double nPoints       = static_cast<double>(magnetizationNearTc.size());
            double slope         = (nPoints * sumLogMT - sumLogM * sumLogT)
                                 / (nPoints * sumLogT2 - sumLogT * sumLogT);
            beta_exponents[j]   = slope;  // β estimate for this magnetic field
        }
    }

    /**
     * @brief Records the β exponent and critical temperature for the zero-field (h = 0) case.
     *
     * The zero-field column is the physically most important one because it corresponds
     * to spontaneous symmetry breaking without an external bias.  This method locates
     * the h = 0 column (snapped into place by the constructor) and copies its β and Tc
     * values into the dedicated summary members averageBetaExponent_h0 and
     * averageCriticalTemperature_h0, which are then written to the output files.
     *
     * Note: the loop that accumulates across numTempSteps is a historical artefact –
     * beta_exponents and critical_temperatures are indexed by h, not T, so the same
     * value is added numTempSteps times and then divided out, leaving the original
     * value unchanged.  The net effect is simply:
     *   averageBetaExponent_h0        = beta_exponents[zeroFieldIndex]
     *   averageCriticalTemperature_h0 = critical_temperatures[zeroFieldIndex]
     */
    void FindAverageBetaExponentAndCritTempAtZeroField() {
        // Locate the field grid point that was snapped to h = 0 by the constructor
        int zeroFieldIndex = -1;
        for (int j = 0; j < numHSteps; j++) {
            if (magnetic_fields[j] == 0.0f) {
                zeroFieldIndex = j;
                break;
            }
        }

        // If the sweep didn't include h = 0 (e.g. hMin > 0 or hMax < 0) return NaN
        if (zeroFieldIndex == -1) {
            averageBetaExponent_h0        = std::numeric_limits<float>::quiet_NaN();
            averageCriticalTemperature_h0 = std::numeric_limits<float>::quiet_NaN();
            return;
        }

        // Accumulate (same value numTempSteps times) then average – net effect: copy
        for (int i = 0; i < numTempSteps; i++) {
            averageBetaExponent_h0        += beta_exponents[zeroFieldIndex];
            averageCriticalTemperature_h0 += critical_temperatures[zeroFieldIndex];
        }

        averageBetaExponent_h0        /= numTempSteps;
        averageCriticalTemperature_h0 /= numTempSteps;
    }

    void runIsingSimulation() {
        runSimulation();
        findCriticalTemperatureAndCalculateBeta();
        FindAverageBetaExponentAndCritTempAtZeroField();
    }

    /**
     * @brief Saves all simulation results to a NumPy-compatible .npz archive.
     *
     * The archive contains the following named arrays (readable with np.load() in Python):
     *
     *   metadata                        float[8]          –  simulation parameters:
     *                                                         [n, iterations, hMin, hMax,
     *                                                          numHSteps, tempMin, tempMax, numTempSteps]
     *   avg_magnetization               double[n_h, n_T]  –  <|m|> for each (h, T) pair
     *   magnetic_susceptibility         double[n_h, n_T]  –  χ for each (h, T) pair
     *   temperatures                    float[n_T]        –  temperature grid
     *   magnetic_fields                 float[n_h]        –  field grid
     *   critical_temperatures           float[n_h]        –  estimated Tc for each h
     *   beta_exponents                  float[n_h]        –  fitted β for each h
     *   average_beta_exponent_h0        float[1]          –  β at h = 0
     *   average_critical_temperature_h0 float[1]          –  Tc at h = 0
     *
     * 2-D arrays are stored in C row-major order: outer index = h, inner index = T.
     * The first array is written with mode "w" (create/overwrite); all subsequent
     * arrays use mode "a" (append to the same archive).
     */
    void saveResultsToNPZ(const std::string& filename) {
        // ── Flatten 2-D result vectors to 1-D for cnpy ───────────────────────
        // cnpy requires raw pointers to contiguous data; we copy from the nested
        // std::vectors into flat buffers with row-major layout [h_idx * n_T + T_idx].
        std::vector<double> avgMagnetizationFlat(numHSteps * numTempSteps);
        std::vector<double> magneticSusceptibilityFlat(numHSteps * numTempSteps);

        // ── Build metadata vector ────────────────────────────────────────────
        // Packing these simulation hyperparameters into the archive makes the .npz
        // file self-describing – the Python analysis script can read them without
        // needing a separate configuration file.
        std::vector<float> metadata;
        metadata.push_back((float)n);          // Lattice side length (actual_n, excludes ghost layers)
        metadata.push_back((float)iterations); // Number of production sweeps per run
        metadata.push_back(hMin);              // Minimum magnetic field value
        metadata.push_back(hMax);              // Maximum magnetic field value
        metadata.push_back((float)numHSteps);  // Number of field grid points
        metadata.push_back(tempMin);           // Minimum temperature value
        metadata.push_back(tempMax);           // Maximum temperature value
        metadata.push_back((float)numTempSteps); // Number of temperature grid points

        // Flatten result arrays in parallel; no race conditions since each (i, j)
        // maps to a unique flat index.
#pragma omp parallel for collapse(2) schedule(static)
        for (int i = 0; i < numHSteps; i++) {
            for (int j = 0; j < numTempSteps; j++) {
                avgMagnetizationFlat[i * numTempSteps + j]        = avg_magnetizations[i][j];
                magneticSusceptibilityFlat[i * numTempSteps + j]  = magnetic_susceptibilities[i][j];
            }
        }

        // ── Write to archive ─────────────────────────────────────────────────
        // "w" creates (or overwrites) the file; "a" appends additional arrays.
        cnpy::npz_save(filename, "metadata",
                       metadata.data(), std::vector<size_t>{metadata.size()}, "w");
        cnpy::npz_save(filename, "avg_magnetization",
                       avgMagnetizationFlat.data(),
                       std::vector<size_t>{(size_t)numHSteps, (size_t)numTempSteps}, "a");
        cnpy::npz_save(filename, "magnetic_susceptibility",
                       magneticSusceptibilityFlat.data(),
                       std::vector<size_t>{(size_t)numHSteps, (size_t)numTempSteps}, "a");
        cnpy::npz_save(filename, "temperatures",
                       temperatures.data(), std::vector<size_t>{(size_t)numTempSteps}, "a");
        cnpy::npz_save(filename, "magnetic_fields",
                       magnetic_fields.data(), std::vector<size_t>{(size_t)numHSteps}, "a");
        cnpy::npz_save(filename, "critical_temperatures",
                       critical_temperatures.data(), std::vector<size_t>{(size_t)numHSteps}, "a");
        cnpy::npz_save(filename, "beta_exponents",
                       beta_exponents.data(), std::vector<size_t>{(size_t)numHSteps}, "a");
        cnpy::npz_save(filename, "average_beta_exponent_h0",
                       &averageBetaExponent_h0, std::vector<size_t>{1}, "a");
        cnpy::npz_save(filename, "average_critical_temperature_h0",
                       &averageCriticalTemperature_h0, std::vector<size_t>{1}, "a");
    }

    void saveResultsToCSV(const std::string& filename) {
        std::ofstream file(filename);
        file << "# This CSV file contains the results of the Ising model simulation.\n";
        file << "# Each row corresponds to a specific temperature and magnetic field combination.\n";
        file << "# n: " << n << "\n# iterations: " << iterations << "\n# hMin: " << hMin << "\n# hMax: " << hMax << "\n# numHSteps: " << numHSteps
             << "\n# tempMin: " << tempMin << "\n# tempMax: " << tempMax << "\n# numTempSteps: " << numTempSteps << "\n";

        file << "# average_beta_exponent_h0: " << averageBetaExponent_h0 << "\n";
        file << "# average_critical_temperature_h0: " << averageCriticalTemperature_h0 << "\n";
        file << "Temperature,MagneticField,AverageMagnetization,MagneticSusceptibility,BetaExponent\n";

        std::vector<std::string> buffers(numTempSteps);

#pragma omp parallel for schedule(static)
        for (int i = 0; i < numTempSteps; i++) {
            std::string& buffer = buffers[i];
            buffer.reserve(numHSteps * 100);
            for (int j = 0; j < numHSteps; j++) {
                buffer += std::to_string(temperatures[i]) + "," + std::to_string(magnetic_fields[j]) + "," +
                          std::to_string(avg_magnetizations[j][i]) + "," + std::to_string(magnetic_susceptibilities[j][i]) + "," +
                          std::to_string(beta_exponents[j]) + "\n";
            }
        }

        for (int i = 0; i < numTempSteps; i++) {
            file << buffers[i];
        }
        file.close();
    }

    void saveResults() {
        std::filesystem::create_directory("output");
        saveResultsToNPZ("output/ising_results.npz");
        saveResultsToCSV("output/ising_results.csv");
    }
};

#endif  // PROCESSING_H