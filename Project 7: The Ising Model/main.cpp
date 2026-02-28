/**
 * @file main.cpp
 * @author Nels Buhrley
 * @date 2026-02-17
 *
 * @brief Entry point for the 3D Ising Model simulation.
 *
 * This program simulates the 3D Ising model using the Metropolis algorithm
 * over a 2D sweep of temperature and external magnetic field values.
 * For each (T, h) pair the average magnetization and magnetic susceptibility
 * are recorded.  The code also fits the critical exponent beta near the
 * phase-transition temperature Tc when h = 0.
 *
 * The results are written to:
 *   output/ising_results.npz  – numpy-compatible binary archive
 *   output/ising_results.csv  – plain-text table
 *
 * Parallelisation is handled by OpenMP (see processing.h for details).
 */

#include <iostream>
#include <vector>
#include <random>
#include <omp.h>
#include "processing.h"

int main() {
    // ── Lattice Parameters ──────────────────────────────────────────────────
    // N: side-length of the cubic spin lattice (N x N x N spins).
    // A ghost/halo layer of width 1 is added internally for periodic BCs,
    // so the true allocated size is (N+2)^3.
    int N = 100;

    // Number of Metropolis sweeps to average over *after* the warmup phase.
    // More iterations → lower statistical noise but longer runtime.
    int iterations = 200;

    // ── Temperature Range ───────────────────────────────────────────────────
    // The simulation sweeps linearly from minTemp to maxTemp.
    // The theoretical critical temperature for the 3D Ising model is
    // approximately Tc ≈ 4.51 (in units of J/kB); adjust the range to bracket it.
    float minTemp  = 0.01;   // Starting (low) temperature
    float maxTemp  = 6.0;    // Ending (high) temperature
    int   tempSteps = 10;    // Number of evenly-spaced temperature grid points

    // ── External Magnetic Field Range ───────────────────────────────────────
    // The field h is swept symmetrically around zero so that the
    // spontaneous magnetisation curve (h = 0) is always included.
    // If hMin < 0 < hMax the constructor will snap the nearest grid point to
    // exactly 0 to ensure a clean h = 0 datum (see Simulation constructor).
    float hmax     = 10;         // Upper field bound
    float hmin     = -hmax;      // Lower field bound (symmetric)
    int   numHSteps = 10;        // Number of evenly-spaced field grid points

    // ── Run the Full Simulation ─────────────────────────────────────────────
    // Simulation orchestrates:
    //   1. runSimulation()                    – parallel (T, h) sweep
    //   2. findCriticalTemperatureAndCalculateBeta() – locate Tc, fit beta
    //   3. FindAverageBetaExponentAndCritTempAtZeroField() – h = 0 averages
    Simulation simulation(N, iterations, hmin, hmax, numHSteps, minTemp, maxTemp, tempSteps);
    simulation.runIsingSimulation();

    // ── Save Results ────────────────────────────────────────────────────────
    // Writes both .npz and .csv files inside the output/ directory.
    simulation.saveResults();

    return 0;
}