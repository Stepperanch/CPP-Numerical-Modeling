#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#define _USE_MATH_DEFINES

#include <omp.h>

#include "cnpy.h"

#ifndef PROCESSING_H
#define PROCESSING_H

/**
 * @brief Element-wise addition of two std::array objects.
 *
 * @param a First array operand.
 * @param b Second array operand.
 *
 * @return A new std::array where each element is the sum of the corresponding elements in a and b.
 *
 * @tparam T The type of the elements in the arrays.
 * @tparam N The size of the arrays.
 */
template <typename T, size_t N>
std::array<T, N> operator+(const std::array<T, N>& a, const std::array<T, N>& b) {
    std::array<T, N> res;
    for (size_t i = 0; i < N; ++i)
        res[i] = a[i] + b[i];
    return res;
}

/**
 * @brief Element-wise multiplication of a std::array by a scalar.
 *
 * @param a The array operand.
 * @param scalar The scalar operand.
 *
 * @return A new std::array where each element is the product of the corresponding element in a and scalar.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The size of the array.
 *
 */
template <typename T, size_t N>
std::array<T, N> operator*(const std::array<T, N>& a, T scalar) {
    std::array<T, N> res;
    for (size_t i = 0; i < N; ++i)
        res[i] = a[i] * scalar;
    return res;
}

/**
 * @brief A helper class for performing unit conversions in quantum mechanical calculations.
 *
 * Set characteristic length and mass scales, then call the calculate methods to derive
 * the corresponding energy and time conversion factors from Planck's constant.
 * Provides methods to convert lengths, masses, energies, and times from reduced units
 * to SI or eV units.
 */
class UnitHelper {
   public:
    double hbar_SI = 1.0545718e-34;    // Planck's constant over 2π in J·s
    double hbar_ev = 6.582119569e-16;  // Planck's constant over 2π in eV·s
    double energyConversionFactor;
    double timeConversionFactor;
    double Lnaught;  // Characteristic length scale in meters
    double Mnaught;  // Characteristic mass scale in kilograms

    /** @brief Sets the characteristic mass scale in kilograms. */
    void setMnaught(double mass) {
        Mnaught = mass;
    }

    /** @brief Sets the characteristic length scale in meters. */
    void setLnaught(double length) {
        Lnaught = length;
    }

    /** @brief Computes energyConversionFactor = ℏ² / (Mnaught · Lnaught²). */
    void calculateEnergyConversionFactor(bool useEvUnits) {
        if (useEvUnits) {
            energyConversionFactor = hbar_ev * hbar_ev / (Mnaught * Lnaught * Lnaught);
        } else {
            energyConversionFactor = hbar_SI * hbar_SI / (Mnaught * Lnaught * Lnaught);
        }
    }

    /** @brief Computes timeConversionFactor = Mnaught · Lnaught² / ℏ. */
    void calculateTimeConversionFactor(bool useEvUnits) {
        if (useEvUnits) {
            timeConversionFactor = Mnaught * Lnaught * Lnaught / hbar_ev;
        } else {
            timeConversionFactor = Mnaught * Lnaught * Lnaught / hbar_SI;
        }
    }

    /** @brief Convenience method that calls all four setters in one call. */
    void setAllConversionFactors(double mass, double length, bool useEvUnits) {
        setMnaught(mass);
        setLnaught(length);
        calculateEnergyConversionFactor(useEvUnits);
        calculateTimeConversionFactor(useEvUnits);
    }

    /** @brief Converts a length from reduced units to meters. */
    inline double convertLengthFromReducedUnits(double length) const {
        return length * Lnaught;
    }
    /** @brief Converts a mass from reduced units to kilograms. */
    inline double convertMassFromReducedUnits(double mass) const {
        return mass * Mnaught;
    }
    /** @brief Converts an energy from reduced units to SI or eV units. */
    inline double convertEnergyFromReducedUnits(double energy) const {
        return energy * energyConversionFactor;
    }
    /** @brief Converts a time from reduced units to seconds. */
    inline double convertTimeFromReducedUnits(double time) const {
        return time * timeConversionFactor;
    }
};

/**
 * @brief A helper class for integrating 1D Schrodinger-form ODEs with the Numerov method.
 *
 * Uses fixed-step Numerov recurrences on a precomputed potential grid. Provides
 * trajectory integration, final-state estimation, and node counting utilities.
 */
class Integrator {
   public:
    double divergenceThreshold = 1e4;  //  d Threshold for detecting divergence in the solution.
    //                                      If |ψ| exceeds this value, the solution is considered to be diverging.

    /** @brief Integrates psi(x) on a fixed grid using Numerov.
     * @param E Trial energy.
     * @param potential Precomputed V(x_i) values on a uniform grid.
     * @param x0 Initial position.
     * @param Y0 Initial state [psi, dpsi/dx].
     * @param xEnd Final position.
     * @param numSteps Number of intervals in [x0, xEnd].
     * @param psiResults Output psi trajectory.
     */
    void NumerovIntegrate(double E, const std::vector<double>& potential, double x0, const std::array<double, 2>& Y0, double xEnd, int numSteps,
                          std::vector<double>& psiResults) const {
        psiResults.clear();                                                           // Clear output vector before starting integration
        if (numSteps < 2 || potential.size() != static_cast<size_t>(numSteps) + 1) {  // Validate input parameters
            return;
        }

        const double dx = (xEnd - x0) / static_cast<double>(numSteps);  // Step size based on the number of intervals
        const double h2 = dx * dx;                                      // Precompute dx² for use in the Numerov formula
        const double divergenceCap = std::max(divergenceThreshold * std::abs(Y0[0]),
                                              static_cast<double>(divergenceThreshold));  // Dynamic divergence threshold based on initial psi value

        auto gAt = [&](int i) -> double {
            return 2.0 * (potential[static_cast<size_t>(i)] - E);
        };  // Lambda to compute g(x_i) = 2(V(x_i) - E) at grid index i

        psiResults.resize(static_cast<size_t>(numSteps) + 1, 0.0);       // Initialize psiResults with the correct size and default values
        psiResults[0] = Y0[0];                                           // Set initial psi value at x0
        psiResults[1] = Y0[0] + dx * Y0[1] + 0.5 * h2 * gAt(0) * Y0[0];  // Compute psi at the first step using Taylor expansion

        if (!std::isfinite(psiResults[0]) || !std::isfinite(psiResults[1])) {  // Check for numerical issues in the initial conditions
            psiResults.clear();
            return;
        }

        int lastValid = 1;
        for (int i = 1; i < numSteps; ++i) {  // Iterate through the grid points to compute psi using the Numerov recurrence relation
            const double gPrev = gAt(i - 1);
            const double gCurr = gAt(i);
            const double gNext = gAt(i + 1);

            // const double aPrev = 1.0 - (h2 / 12.0) * gPrev;
            // const double aCurr = 1.0 + (5.0 * h2 / 12.0) * gCurr;
            // const double aNext = 1.0 - (h2 / 12.0) * gNext;

            const double aPrev = 1.0 - (h2 / 12.0) * gPrev;
            const double aCurr = 1.0 + (5.0 * h2 / 12.0) * gCurr;
            const double aNext = 1.0 - (h2 / 12.0) * gNext;

            if (std::abs(aNext) < 1e-14) {  // Avoid division by near-zero aNext which can cause numerical instability.
                break;
            }

            const double psiNext = (2.0 * aCurr * psiResults[static_cast<size_t>(i)] - aPrev * psiResults[static_cast<size_t>(i - 1)]) / aNext;
            if (!std::isfinite(psiNext)) {  // Check for NaN or Inf which indicates numerical instability.
                break;
            }

            psiResults[static_cast<size_t>(i + 1)] = psiNext;
            lastValid = i + 1;
            if (std::abs(psiNext) > divergenceCap) {  // Check for divergence based on the dynamic threshold.
                break;
            }
        }

        psiResults.resize(static_cast<size_t>(lastValid) + 1);  // Resize the results vector to include only the valid computed points.
    }
};

/** @brief A system for testing quantum mechanical calculations.
 * The potential is V(x) = x^n / n, and the Schrodinger equation is solved for a given energy level E using Numerov.
 */
class QuantumTestSystem {
   public:
    int n;
    double E;  // Energy level for which we want to solve the Schrodinger equation

    /** @brief Constructs a quantum test system with the given potential and energy level.
     * @param n The power of the potential function.
     * @param E The energy level for which to solve the Schrodinger equation.
     */
    QuantumTestSystem(int n_, double E_) : n(n_), E(E_) {}

    /** @brief Computes the potential function V(x).
     * @param x The position at which to evaluate the potential.
     * @return The value of the potential at x.
     */
    inline double V(double x) const {
        return std::pow(x, n) / n;
    }

    /** @brief Computes the derivative helper function.
     * @param x The position at which to evaluate the derivative.
     * @return The value of the derivative helper at x.
     */
    inline double derivhelper(double x) const {
        return 2 * (V(x) - E);
    }

    /** @brief Computes the derivative function.
     * @param x The position at which to evaluate the derivative.
     * @param Yvec The state vector at x.
     * @return The value of the derivative at x.
     */
    inline std::array<double, 2> derivativeFunc(double x, const std::array<double, 2>& Yvec) const {
        return std::array<double, 2>{Yvec[1], derivhelper(x) * Yvec[0]};
    }

    /** @brief Builds a potential grid V(x_i) for Numerov integration.
     * @param x0 The initial position.
     * @param xEnd The final position.
     * @param numSteps Number of uniform intervals.
     * @return V(x_i) values with size numSteps + 1.
     */
    std::vector<double> buildPotentialGrid(double x0, double xEnd, int numSteps) const {
        std::vector<double> potential(static_cast<size_t>(numSteps) + 1);
        const double dx = (xEnd - x0) / static_cast<double>(numSteps);
        for (int i = 0; i <= numSteps; ++i) {
            const double x = x0 + static_cast<double>(i) * dx;
            potential[static_cast<size_t>(i)] = V(x);
        }
        return potential;
    }

    /** @brief Solves the Schrodinger equation for the given initial conditions and parameters using Numerov.
     * @param x0 The initial position.
     * @param Y0 The initial state vector.
     * @param xEnd The final position.
     * @param h0 Step size hint used to build a fixed Numerov grid.
     * @return The final state at xEnd (or at divergence) on success, std::nullopt on numerical failure.
     */
    std::optional<std::array<double, 2>> solve(double x0, std::array<double, 2> Y0, double xEnd, double h0 = 0.01) const {
        if (h0 <= 0.0) {
            return std::nullopt;
        }
        const int numSteps = std::max(2, static_cast<int>(std::ceil(std::abs((xEnd - x0) / h0))));
        return solve(x0, Y0, xEnd, numSteps);
    }

    /** @brief Computes the full trajectory using fixed-step Numerov.
     * @param x0 The initial position.
     * @param Y0 The initial state vector.
     * @param xEnd The final position.
     * @param numSteps The number of steps to take.
     * @param results A vector to store the trajectory points.
     */
    void solve(double x0, std::array<double, 2> Y0, double xEnd, int numSteps, std::vector<std::array<double, 2>>& results) const {
        results.clear();
        if (numSteps < 2) {
            return;
        }

        Integrator integrator;
        const std::vector<double> potential = buildPotentialGrid(x0, xEnd, numSteps);

        std::vector<double> psi;
        integrator.NumerovIntegrate(E, potential, x0, Y0, xEnd, numSteps, psi);
        if (psi.empty()) {
            return;
        }

        const double dx = (xEnd - x0) / static_cast<double>(numSteps);
        results.resize(psi.size());
        for (size_t i = 0; i < psi.size(); ++i) {
            double dpsi = 0.0;
            if (i == 0) {
                dpsi = Y0[1];
            } else if (i + 1 < psi.size()) {
                dpsi = (psi[i + 1] - psi[i - 1]) / (2.0 * dx);
            } else {
                dpsi = (psi[i] - psi[i - 1]) / dx;
            }
            results[i] = {psi[i], dpsi};
        }
    }

    /** @brief Returns only psi(xEnd) using fixed-step Numerov.
     * @param x0 The initial position.
     * @param Y0 The initial state vector [psi, dpsi/dx].
     * @param xEnd The final position.
     * @param h0 Initial step size hint.
     * @return psi(xEnd) (or divergence psi) if successful, std::nullopt on solver failure.
     */
    std::optional<double> solvePsi(double x0, std::array<double, 2> Y0, double xEnd, double h0 = 0.01) const {
        const auto finalState = solve(x0, Y0, xEnd, h0);
        if (!finalState) {
            return std::nullopt;
        }
        return (*finalState)[0];
    }

    /** @brief Returns only the psi trajectory using fixed-step Numerov.
     * @param x0 The initial position.
     * @param Y0 The initial state vector [psi, dpsi/dx].
     * @param xEnd The final position.
     * @param numSteps Number of Numerov steps.
     * @param psiResults Output vector containing psi values at each step.
     */
    void solvePsi(double x0, std::array<double, 2> Y0, double xEnd, int numSteps, std::vector<double>& psiResults) const {
        Integrator integrator;
        const std::vector<double> potential = buildPotentialGrid(x0, xEnd, numSteps);
        integrator.NumerovIntegrate(E, potential, x0, Y0, xEnd, numSteps, psiResults);
    }
};

/**
 * @brief A helper class for bracketing energy eigenvalues based on node counting.
 * Stores pairs of energies that bracket a node transition along with the node count at the transition.
 */
class NodalBracket {
   public:
    double plusEnergy;
    double minusEnergy;
    int node;

    NodalBracket(double plusEnergy_, double minusEnergy_, int node_) : plusEnergy(plusEnergy_), minusEnergy(minusEnergy_), node(node_) {}
};

/**
 * @brief A class for performing energy sweeps to find energy eigenstates of the quantum test system.
 * Sweeps through energy values, counts nodes in the wavefunction, and collects brackets where node transitions occur.
 */
class Eigenstate {
   public:
    double energy;
    int number;
    std::vector<double> psiTrajectory;
};

/**
 * @brief Performs an energy sweep to find the first 8 energy eigenstates of the quantum test system.
 * @param n The power of the potential function V(x) = x^n / n.
 * @param nodesToFind The number of energy eigenstates to find, determined by counting nodes in the wavefunction.
 * @return A vector of the first "nodesToFind" energy eigenstates found.
 *
 */
class ESweep {
   public:
    int n;
    int nodesToFind;  // Number of energy eigenstates to find, determined by counting nodes in the wavefunction
    double x0 = 0.0;
    double xEnd = 8.0;

    ESweep(int n_, int nodesToFind_ = 10) : n(n_), nodesToFind(nodesToFind_), x0(0.0) {}

    /** @brief Removes obvious divergence tails from a Numerov trajectory.
     *  Detects sudden amplitude blow-up followed by persistent growth and trims from that point.
     */
    void trimNumericalInstability(std::vector<double>& psi, bool aggressive) const {
        if (psi.size() < 8) {
            return;
        }

        std::vector<double> absPsi(psi.size(), 0.0);
        for (size_t i = 0; i < psi.size(); ++i) {
            absPsi[i] = std::abs(psi[i]);
        }

        const double blowupRatio = 50.0;
        double runningMax = std::max(1e-14, absPsi[0]);
        size_t trimStart = psi.size();

        for (size_t i = 1; i + 2 < absPsi.size(); ++i) {
            if (absPsi[i] > blowupRatio * runningMax && absPsi[i + 2] > absPsi[i + 1] && absPsi[i + 1] > absPsi[i]) {
                trimStart = i;
                break;
            }
            runningMax = std::max(runningMax, absPsi[i]);
        }

        if (!aggressive) {
            // For even states, only trim unmistakable catastrophic divergence.
            if (trimStart < psi.size() && trimStart >= 3 && absPsi.back() > 100.0 * std::max(1e-14, runningMax)) {
                psi.resize(trimStart);
            }
            return;
        }

        // Also trim sustained monotonic growth tails (common forbidden-region instability pattern).
        if (absPsi.size() >= 10) {
            size_t growthStart = absPsi.size() - 1;
            while (growthStart > 0 && absPsi[growthStart] > absPsi[growthStart - 1]) {
                --growthStart;
            }

            const size_t suffixLen = absPsi.size() - growthStart;
            if (suffixLen >= 8 && growthStart > absPsi.size() / 3) {
                double prefixMax = 0.0;
                for (size_t i = 0; i < growthStart; ++i) {
                    prefixMax = std::max(prefixMax, absPsi[i]);
                }
                if (prefixMax > 0.0 && absPsi.back() > 2.0 * prefixMax) {
                    trimStart = std::min(trimStart, growthStart);
                }
            }
        }

        // Envelope-minimum criterion: if the right tail rises far above a late minimum,
        // cut from the point where growth clearly restarts.
        const size_t startWindow = absPsi.size() / 4;
        if (startWindow + 8 < absPsi.size()) {
            size_t minIdx = startWindow;
            for (size_t i = startWindow + 1; i < absPsi.size(); ++i) {
                if (absPsi[i] < absPsi[minIdx]) {
                    minIdx = i;
                }
            }

            const double minVal = std::max(1e-15, absPsi[minIdx]);
            if (minIdx + 6 < absPsi.size() && absPsi.back() > 20.0 * minVal) {
                for (size_t j = minIdx + 1; j + 3 < absPsi.size(); ++j) {
                    if (absPsi[j] > 5.0 * minVal && absPsi[j + 1] > absPsi[j] && absPsi[j + 2] > absPsi[j + 1] && absPsi[j + 3] > absPsi[j + 2]) {
                        trimStart = std::min(trimStart, j);
                        break;
                    }
                }
            }
        }

        // Long increasing-run criterion: trim when the tail increases for many consecutive points.
        const size_t runLength = 24;
        size_t runStart = absPsi.size();
        size_t runCount = 0;
        for (size_t i = 1; i < absPsi.size(); ++i) {
            if (absPsi[i] > absPsi[i - 1]) {
                if (runCount == 0) {
                    runStart = i - 1;
                }
                ++runCount;
                if (runCount >= runLength && runStart > absPsi.size() / 6) {
                    trimStart = std::min(trimStart, runStart);
                    break;
                }
            } else {
                runCount = 0;
            }
        }

        if (trimStart < psi.size() && trimStart >= 3) {
            psi.resize(trimStart);
        }
    }

    /** @brief Normalizes psi so the full mirrored wavefunction has unit norm when x0=0.
     *  For half-domain parity solutions, integral over full line is 2 * integral over [0, +inf).
     */
    void normalizeTrajectory(std::vector<double>& psi, int trajectorySteps) const {
        if (psi.size() < 2 || trajectorySteps <= 0) {
            return;
        }

        const double dx = std::abs((xEnd - x0) / static_cast<double>(trajectorySteps));
        if (!(dx > 0.0)) {
            return;
        }

        double halfIntegral = 0.0;
        for (size_t i = 1; i < psi.size(); ++i) {
            const double y0 = psi[i - 1] * psi[i - 1];
            const double y1 = psi[i] * psi[i];
            halfIntegral += 0.5 * (y0 + y1) * dx;
        }

        if (!(halfIntegral > 0.0) || !std::isfinite(halfIntegral)) {
            return;
        }

        // If we start at x=0, this trajectory is one parity half of the full wavefunction.
        const bool isHalfDomainFromCenter = std::abs(x0) < 1e-12;
        const double fullIntegral = isHalfDomainFromCenter ? (2.0 * halfIntegral) : halfIntegral;

        if (!(fullIntegral > 0.0) || !std::isfinite(fullIntegral)) {
            return;
        }

        const double scale = 1.0 / std::sqrt(fullIntegral);
        for (double& value : psi) {
            value *= scale;
        }
    }

    /** @brief Counts the number of nodes in a wavefunction.
     * @param state The wavefunction state vector.
     * @return The number of nodes.
     */
    int countNodes(const std::vector<double>& psi) const {
        if (psi.empty()) {
            return 0;
        }

        double maxAbsPsi = 0.0;
        for (double value : psi) {
            maxAbsPsi = std::max(maxAbsPsi, std::abs(value));
        }

        // Ignore tiny numerical sign chatter near zero when counting nodes.
        const double tolerance = std::max(1e-12, 1e-8 * maxAbsPsi);

        int nodes = 0;
        int lastSign = 0;
        for (double value : psi) {
            if (std::abs(value) <= tolerance) {
                continue;
            }

            const int sign = (value > 0.0) ? 1 : -1;
            if (lastSign != 0 && sign != lastSign) {
                ++nodes;
            }
            lastSign = sign;
        }

        return nodes;
    }

    /** @brief Solves at a trial energy and returns its node count from a full trajectory.
     * @param E Trial energy.
     * @param trajectorySteps Number of fixed Numerov steps to resolve node crossings.
     * @param evenParity If true, use even-parity initial conditions {1,0}; otherwise odd-parity {0,1}.
     * @return Node count on success, std::nullopt on solver failure.
     */
    std::optional<int> solveNodeCount(double E, int trajectorySteps, bool evenParity) const {
        QuantumTestSystem system(n, E);
        std::vector<double> psi;

        if (xEnd <= x0 + 1e-10) {
            return 0;
        }

        // Use even or odd parity initial conditions.
        const std::array<double, 2> ic = evenParity ? std::array<double, 2>{1.0, 0.0} : std::array<double, 2>{0.0, 1.0};
        system.solvePsi(x0, ic, xEnd, trajectorySteps, psi);

        if (psi.size() < 2) {
            return std::nullopt;
        }
        return countNodes(psi);
    }

    /** @brief Builds a reusable potential grid V(x_i) for the sweep interval.
     * @param trajectorySteps Number of intervals in [x0, xEnd].
     * @return Vector containing V on a uniform grid of size trajectorySteps + 1.
     */
    std::vector<double> buildPotentialGrid(int trajectorySteps) const {
        std::vector<double> potential(static_cast<size_t>(trajectorySteps) + 1);
        const double dx = (xEnd - x0) / static_cast<double>(trajectorySteps);
        for (int i = 0; i <= trajectorySteps; ++i) {
            const double x = x0 + static_cast<double>(i) * dx;
            potential[static_cast<size_t>(i)] = std::pow(x, n) / n;
        }
        return potential;
    }

    /** @brief Sweeps one parity channel and collects up to maxToFind node-transition brackets.
     * @param evenParity True for even-parity {1,0} ICs, false for odd-parity {0,1} ICs.
     * @param maxToFind Maximum number of brackets to collect in this channel.
     * @param E_min Starting energy.
     * @param E_h Energy step size.
     * @param E_max Upper energy limit.
     * @param trajectorySteps Numerov steps per solve.
     * @param maxIterations Safety cap on loop iterations.
     * @return Vector of (minusEnergy, plusEnergy) pairs where a node transition was detected.
     */
    std::vector<std::pair<double, double>> sweepParity(bool evenParity, int maxToFind, double E_min, double E_h, double E_max, int trajectorySteps,
                                                       int maxIterations) const {
        std::vector<std::pair<double, double>> transitions;
        transitions.reserve(static_cast<size_t>(maxToFind));

        int lastNodeCount = -1;
        double E = E_min;
        for (int iter = 0; iter < maxIterations && E <= E_max; ++iter, E += E_h) {
            const auto nc = solveNodeCount(E, trajectorySteps, evenParity);
            if (!nc)
                continue;

            if (lastNodeCount != -1 && *nc > lastNodeCount) {
                const int jump = *nc - lastNodeCount;
                for (int s = 1; s <= jump && static_cast<int>(transitions.size()) < maxToFind; ++s) {
                    transitions.emplace_back(E - E_h, E);
                }
            }
            if (lastNodeCount == -1 || *nc > lastNodeCount)
                lastNodeCount = *nc;
            if (static_cast<int>(transitions.size()) >= maxToFind)
                break;
        }
        return transitions;
    }

    /** @brief Finds nodal brackets by running separate even- and odd-parity sweeps then
     *  merging and sorting by energy, yielding the full ordered spectrum.
     */
    std::vector<NodalBracket> findNodalBrackets(double E_min, double E_h, double E_max = std::numeric_limits<double>::infinity(),
                                                int trajectorySteps = 2000, int maxIterations = 200000) const {
        if (nodesToFind <= 0 || E_h <= 0.0 || trajectorySteps < 2 || maxIterations <= 0) {
            return {};
        }

        // Each parity channel contributes half the states (ceil for even, floor for odd).
        const int evenCount = (nodesToFind + 1) / 2;
        const int oddCount = nodesToFind / 2;

        const auto evenTransitions = sweepParity(true, evenCount, E_min, E_h, E_max, trajectorySteps, maxIterations);
        const auto oddTransitions = sweepParity(false, oddCount, E_min, E_h, E_max, trajectorySteps, maxIterations);

        // Merge both channels into one list sorted by the lower bound of each bracket.
        std::vector<std::pair<double, double>> all;
        all.insert(all.end(), evenTransitions.begin(), evenTransitions.end());
        all.insert(all.end(), oddTransitions.begin(), oddTransitions.end());
        std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        // Assign sequential node labels (1-indexed) after sorting by energy.
        std::vector<NodalBracket> brackets;
        brackets.reserve(all.size());
        for (int i = 0; i < static_cast<int>(all.size()); ++i) {
            brackets.emplace_back(all[static_cast<size_t>(i)].second, all[static_cast<size_t>(i)].first, i + 1);
        }
        return brackets;
    }

    std::vector<Eigenstate> findEigenstates(const std::vector<NodalBracket>& brackets, int trajectorySteps = 2000, int maxBisectionIterations = 1000,
                                            double convergenceThreshold = 1e-10) const {
        std::vector<Eigenstate> eigenstates(brackets.size());

#pragma omp parallel for
        for (int idx = 0; idx < static_cast<int>(brackets.size()); ++idx) {
            const auto& bracket = brackets[static_cast<size_t>(idx)];
            double E_low = bracket.minusEnergy;
            double E_high = bracket.plusEnergy;
            double E_mid = 0.5 * (E_low + E_high);
            const int targetNodes = bracket.node;
            // Quantum number is 0-indexed: node label 1 = n=0 (even), 2 = n=1 (odd), ...
            const bool evenParity = ((targetNodes - 1) % 2 == 0);

            // Map merged spectrum index to parity-channel index used by solveNodeCount.
            const int targetInParityChannel = evenParity ? ((targetNodes + 1) / 2) : (targetNodes / 2);

            const std::array<double, 2> ic = evenParity ? std::array<double, 2>{1.0, 0.0} : std::array<double, 2>{0.0, 1.0};

            auto endpointPsi = [&](double E_trial) -> std::optional<double> {
                QuantumTestSystem trialSystem(n, E_trial);
                std::vector<double> psiTrial;
                trialSystem.solvePsi(x0, ic, xEnd, trajectorySteps, psiTrial);
                if (psiTrial.empty()) {
                    return std::nullopt;
                }
                return psiTrial.back();
            };

            auto psiLowOpt = endpointPsi(E_low);
            auto psiHighOpt = endpointPsi(E_high);
            const bool hasResidualBracket = psiLowOpt.has_value() && psiHighOpt.has_value() && std::isfinite(*psiLowOpt) &&
                                            std::isfinite(*psiHighOpt) && ((*psiLowOpt) * (*psiHighOpt) <= 0.0);

            for (int i = 0; i < maxBisectionIterations; ++i) {
                E_mid = 0.5 * (E_low + E_high);
                if (std::abs(E_high - E_low) < convergenceThreshold)
                    break;

                if (hasResidualBracket) {
                    const auto psiMidOpt = endpointPsi(E_mid);
                    if (!psiMidOpt || !std::isfinite(*psiMidOpt)) {
                        break;
                    }

                    if (std::abs(*psiMidOpt) < convergenceThreshold) {
                        break;
                    }

                    if ((*psiLowOpt) * (*psiMidOpt) <= 0.0) {
                        E_high = E_mid;
                        psiHighOpt = psiMidOpt;
                    } else {
                        E_low = E_mid;
                        psiLowOpt = psiMidOpt;
                    }
                } else {
                    // Fallback: minimize |psi(xEnd)| inside the bracket when no sign change is present.
                    // This better enforces the decaying boundary condition than node-count fallback.
                    constexpr int samples = 48;
                    double bestE = E_mid;
                    double bestAbsResidual = std::numeric_limits<double>::infinity();

                    for (int s = 0; s <= samples; ++s) {
                        const double alpha = static_cast<double>(s) / static_cast<double>(samples);
                        const double E_test = E_low + alpha * (E_high - E_low);
                        const auto psiTestOpt = endpointPsi(E_test);
                        if (!psiTestOpt || !std::isfinite(*psiTestOpt)) {
                            continue;
                        }

                        const double absResidual = std::abs(*psiTestOpt);
                        if (absResidual < bestAbsResidual) {
                            bestAbsResidual = absResidual;
                            bestE = E_test;
                        }
                    }

                    E_mid = bestE;

                    // Shrink around the current best point to refine subsequent iterations.
                    const double halfWidth = std::max(0.25 * (E_high - E_low), convergenceThreshold);
                    E_low = std::max(bracket.minusEnergy, E_mid - halfWidth);
                    E_high = std::min(bracket.plusEnergy, E_mid + halfWidth);

                    if (bestAbsResidual < convergenceThreshold) {
                        break;
                    }
                }
            }

            Eigenstate state;
            state.energy = E_mid;
            state.number = targetNodes - 1;
            QuantumTestSystem system(n, E_mid);
            system.solvePsi(x0, ic, xEnd, trajectorySteps, state.psiTrajectory);

            trimNumericalInstability(state.psiTrajectory, !evenParity);
            normalizeTrajectory(state.psiTrajectory, trajectorySteps);

            eigenstates[static_cast<size_t>(targetNodes - 1)] = std::move(state);  // Write to unique slot: thread-safe.
        }

        return eigenstates;
    }

    std::vector<Eigenstate> findEigenstates(double E_min, double E_h, double E_max = std::numeric_limits<double>::infinity(),
                                            int trajectorySteps = 2000, int maxIterations = 200000, int maxBisectionIterations = 1000,
                                            double convergenceThreshold = 1e-10) const {
        const auto brackets = findNodalBrackets(E_min, E_h, E_max, trajectorySteps, maxIterations);
        return findEigenstates(brackets, trajectorySteps, maxBisectionIterations, convergenceThreshold);
    }
};

class EigenstateFinder {
   public:
    int maxN;
    int nodesToFind;

    EigenstateFinder(int maxN_, int nodesToFind_ = 10) : maxN(maxN_), nodesToFind(nodesToFind_) {}

    std::map<int, std::vector<Eigenstate>> findEigenstatesForAllN(double E_min, double E_h, double E_max = std::numeric_limits<double>::infinity(),
                                                                  int trajectorySteps = 2000, int maxIterations = 200000,
                                                                  int maxBisectionIterations = 1000, double convergenceThreshold = 1e-10) const {
        std::map<int, std::vector<Eigenstate>> allEigenstates;
        for (int n = 2; n <= maxN; ++n) {
            ESweep sweep(n, nodesToFind);
            const auto eigenstates =
                sweep.findEigenstates(E_min, E_h, E_max, trajectorySteps, maxIterations, maxBisectionIterations, convergenceThreshold);
            allEigenstates[n] = std::move(eigenstates);
        }
        return allEigenstates;
    }
};

class outHelper {
   public:
    std::filesystem::path outputDir;

    explicit outHelper(std::filesystem::path outputDir_) : outputDir(std::move(outputDir_)) {
        std::filesystem::create_directories(outputDir);
    }

    void saveEigenstates(const std::map<int, std::vector<Eigenstate>>& allEigenstates) const {
        const std::filesystem::path archivePath = outputDir / "eigenstates.npz";

        bool wroteAny = false;
        for (const auto& [n, eigenstates] : allEigenstates) {
            for (size_t idx = 0; idx < eigenstates.size(); ++idx) {
                const auto& state = eigenstates[idx];
                if (state.psiTrajectory.empty()) {
                    continue;
                }

                const std::string varName = "n_" + std::to_string(n) + "_state_" + std::to_string(state.number) + "_idx_" + std::to_string(idx);
                const char* mode = wroteAny ? "a" : "w";
                cnpy::npz_save(archivePath.string(), varName, state.psiTrajectory.data(), {state.psiTrajectory.size()}, mode);
                wroteAny = true;
            }
        }

        // Ensure the archive exists even if all trajectories were empty.
        if (!wroteAny) {
            const std::array<double, 1> emptyMarker{0.0};
            cnpy::npz_save(archivePath.string(), "empty", emptyMarker.data(), {static_cast<size_t>(0)}, "w");
        }
    }
};
#endif  // PROCESSING_H