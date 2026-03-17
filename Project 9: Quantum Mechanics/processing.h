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
    void NumerovIntegrate(double E, const std::vector<double>& potential, double x0, const std::array<double, 2>& Y0,
                                                      double xEnd, int numSteps, std::vector<double>& psiResults) const {
        psiResults.clear();  // Clear output vector before starting integration
        if (numSteps < 2 || potential.size() != static_cast<size_t>(numSteps) + 1) { // Validate input parameters
            return;
        }

        const double dx = (xEnd - x0) / static_cast<double>(numSteps); // Step size based on the number of intervals
        const double h2 = dx * dx; // Precompute dx² for use in the Numerov formula
        const double divergenceCap = std::max(divergenceThreshold * std::abs(Y0[0]), static_cast<double>(divergenceThreshold)); // Dynamic divergence threshold based on initial psi value

        auto gAt = [&](int i) -> double { return 2.0 * (potential[static_cast<size_t>(i)] - E); }; // Lambda to compute g(x_i) = 2(V(x_i) - E) at grid index i

        psiResults.resize(static_cast<size_t>(numSteps) + 1, 0.0);  // Initialize psiResults with the correct size and default values
        psiResults[0] = Y0[0]; // Set initial psi value at x0
        psiResults[1] = Y0[0] + dx * Y0[1] + 0.5 * h2 * gAt(0) * Y0[0]; // Compute psi at the first step using Taylor expansion

        if (!std::isfinite(psiResults[0]) || !std::isfinite(psiResults[1])) { // Check for numerical issues in the initial conditions
            psiResults.clear();
            return;
        }

        int lastValid = 1;
        for (int i = 1; i < numSteps; ++i) { // Iterate through the grid points to compute psi using the Numerov recurrence relation
            const double gPrev = gAt(i - 1);
            const double gCurr = gAt(i);
            const double gNext = gAt(i + 1);

            const double aPrev = 1.0 + (h2 / 12.0) * gPrev;
            const double aCurr = 1.0 - (5.0 * h2 / 12.0) * gCurr;
            const double aNext = 1.0 + (h2 / 12.0) * gNext;

            if (std::abs(aNext) < 1e-14) { // Avoid division by near-zero aNext which can cause numerical instability.
                break;
            }

            const double psiNext = (2.0 * aCurr * psiResults[static_cast<size_t>(i)] - aPrev * psiResults[static_cast<size_t>(i - 1)]) / aNext;
            if (!std::isfinite(psiNext)) { // Check for NaN or Inf which indicates numerical instability.
                break;
            }

            psiResults[static_cast<size_t>(i + 1)] = psiNext;
            lastValid = i + 1;
            if (std::abs(psiNext) > divergenceCap) { // Check for divergence based on the dynamic threshold.
                break;
            }
        }

        psiResults.resize(static_cast<size_t>(lastValid) + 1); // Resize the results vector to include only the valid computed points.
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

class NodalBracket {
   public:
    double plusEnergy;
    double minusEnergy;
    int node;

    NodalBracket(double plusEnergy_, double minusEnergy_, int node_) : plusEnergy(plusEnergy_), minusEnergy(minusEnergy_), node(node_) {}
};

class ESweep {
   public:
    int n;
    int nodesToFind;  // Number of energy eigenstates to find, determined by counting nodes in the wavefunction
    double x0;
    std::array<double, 2> Y0;
    double xEnd;
    double h0;

    ESweep(int n_, int nodesToFind_ = 8) : n(n_), nodesToFind(nodesToFind_) {}

    /**
     * @brief Performs an energy sweep to find the first 8 energy eigenstates of the quantum test system.
     * @param x0 The initial position.
     * @param Y0 The initial state vector.
     * @param xEnd The final position.
     * @param h0 The initial step size.
     * @return A vector of the first 8 energy eigenstates found.
     */

    /** @brief Counts the number of nodes in a wavefunction.
     * @param state The wavefunction state vector.
     * @return The number of nodes.
     */
    int countNodes(const std::vector<double>& psi) const {
        int nodes = 0;
        for (size_t i = 1; i < psi.size(); ++i) {
            if (psi[i - 1] * psi[i] < 0.0) {
                ++nodes;
            }
        }
        return nodes;
    }

    /** @brief Solves at a trial energy and returns its node count from a full trajectory.
     * @param E Trial energy.
     * @param trajectorySteps Number of fixed Numerov steps to resolve node crossings.
     * @return Node count on success, std::nullopt on solver failure.
     */
    std::optional<int> solveNodeCount(double E, int trajectorySteps) const {
        QuantumTestSystem system(n, E);
        std::vector<double> psi;
        system.solvePsi(x0, Y0, xEnd, trajectorySteps, psi);
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

    /** @brief Uses bisection to find energy eigenstates by counting nodes in the wavefunction.
     *  this method rapidly processed the energy sweep results from a coarse Numerov sweep to find
     *  energy levels bracketing to the first 8 eigenstates, which are then returned as a
     *  vector of NodalBrackets.
     */
    std::vector<NodalBracket> findNodalBrackets(double E_min, double E_h, double E_max = std::numeric_limits<double>::infinity(),
                                                int trajectorySteps = 2000, int maxIterations = 200000) const {
        std::vector<NodalBracket> brackets;
        brackets.reserve(static_cast<size_t>(nodesToFind));

        if (nodesToFind <= 0 || E_h <= 0.0 || trajectorySteps < 2 || maxIterations <= 0) {
            return brackets;
        }

        int lastNodeCount = -1;
        double E = E_min;
        int iterations = 0;
        const std::vector<double> potential = buildPotentialGrid(trajectorySteps);
        Integrator integrator;

        while (brackets.size() < static_cast<size_t>(nodesToFind) && E <= E_max && iterations < maxIterations) {
            const auto nodeCountOpt = integrator.NumerovCountNodesOnPotentialGrid(E, potential, x0, Y0, xEnd, trajectorySteps);
            if (!nodeCountOpt) {
                E += E_h;
                ++iterations;
                continue;  // Skip this energy level if the solver failed
            }
            const int nodeCount = *nodeCountOpt;

            if (lastNodeCount != -1 && nodeCount > lastNodeCount) {
                const int jump = nodeCount - lastNodeCount;
                for (int missed = 1; missed <= jump && brackets.size() < static_cast<size_t>(nodesToFind); ++missed) {
                    brackets.emplace_back(E, E - E_h, lastNodeCount + missed);  // Bracket for each crossed node level in this energy interval
                }
            }

            lastNodeCount = nodeCount;
            E += E_h;
            ++iterations;
        }

        return brackets;
    }
};
#endif  // PROCESSING_H