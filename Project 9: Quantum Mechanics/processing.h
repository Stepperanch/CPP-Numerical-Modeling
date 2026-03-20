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

/** @brief Computes the potential function.
 * @param x The position at which to evaluate the potential.
 * @param degree The degree of the potential.
 * @return The value of the potential at x.
 */
inline double V(double x, int degree) {
    return std::pow(x, degree) / degree;
}

/** @brief Builds a potential Mesh V(x_i) for Numerov integration.
 * @param x0 The initial position.
 * @param xEnd The final position.
 * @param numSteps Number of uniform intervals.
 * @return V(x_i) values with size numSteps + 1.
 */
std::vector<double> build2TimesPotentialMesh(int degree, double xEnd, double stepSize) {
    int numSteps = static_cast<int>(xEnd / stepSize);
    stepSize = xEnd / static_cast<double>(numSteps);  // Recalculate step size to ensure it divides xEnd evenly.

    std::vector<double> potential(static_cast<size_t>(numSteps) + 1);
    const double dx = (xEnd) / static_cast<double>(numSteps);
    for (int i = 0; i <= numSteps; ++i) {
        const double x = static_cast<double>(i) * dx;
        potential[static_cast<size_t>(i)] = 2.0 * V(x, degree);
    }
    return potential;
}

/** @brief Integrates the 1D Schrodinger equation using the Numerov method.
 * @param E The energy level.
 * @param degree The degree of the potential.
 * @param xEnd The final position.
 * @param numSteps Number of uniform intervals.
 * @param Y0 The initial state vector. Y0[0] = psi(x0), Y0[1] = psi'(x0).
 * @param divergenceThreshold The threshold for detecting divergence.
 * @param potential The precomputed potential values at the grid points.
 * @return A vector containing the integrated psi values at each grid point.
 */
std::vector<double> NumerovIntegrate(double ETimesTwo, int degree, double stepSize, double xEnd, const std::array<double, 2>& Y0,
                                     const std::vector<double>& potentialTimesTwo, double divergenceThreshold = 1e4) {
    int numSteps = static_cast<int>(xEnd / stepSize);
    stepSize = xEnd / static_cast<double>(numSteps);  // Recalculate step size to ensure it divides xEnd evenly.

    const double dx = stepSize;
    const double h2 = dx * dx;

    /** @brief Lambda to compute g(x_i) = 2(V(x_i) - E) at grid index i.
     * Uses precomputed potential values for efficiency.
     */
    auto gAt = [&](int i) -> double {
        return potentialTimesTwo[static_cast<size_t>(i)] - ETimesTwo;
    };  // Lambda to compute g(x_i) = 2(V(x_i) - E) at grid index i

    std::vector<double> psiResults(static_cast<size_t>(numSteps) + 1);  // Vector to store psi values at each grid point

    psiResults[0] = Y0[0];                                           // Set initial psi value at x0
    psiResults[1] = Y0[0] + dx * Y0[1] + 0.5 * h2 * gAt(0) * Y0[0];  // Compute psi at the first step using Taylor expansion

    if (!std::isfinite(psiResults[0]) || !std::isfinite(psiResults[1])) {  // Check for numerical issues in the initial conditions
        psiResults.clear();
        return psiResults;
    }

    double aPrev;
    double aCurr;
    double aNext;

    double psiNext;

    int lastValid = 1;
    for (int i = 1; i < numSteps; ++i) {  // Iterate through the grid points to compute psi using the Numerov recurrence relation

        // Precompute the coefficients for the Numerov formula to improve readability and efficiency.
        aPrev = 1.0 - (h2 / 12.0) * gAt(i - 1);
        aCurr = 1.0 + (5.0 * h2 / 12.0) * gAt(i);
        aNext = 1.0 - (h2 / 12.0) * gAt(i + 1);

        if (std::abs(aNext) < 1e-14) {  // Avoid division by near-zero aNext which can cause numerical instability.
            break;
        }

        psiNext = (2.0 * aCurr * psiResults[static_cast<size_t>(i)] - aPrev * psiResults[static_cast<size_t>(i - 1)]) / aNext;

        psiResults[static_cast<size_t>(i + 1)] = psiNext;

        lastValid = i + 1;
        if (std::abs(psiNext) > divergenceThreshold) {  // Check for divergence based on the dynamic threshold.
            break;
        }
    }
    psiResults.resize(static_cast<size_t>(lastValid) + 1);  // Resize the results vector to include only the valid computed points.
    return psiResults;
}

/**
 * @brief A helper class for bracketing energy eigenvalues based on node counting.
 * Stores pairs of energies that bracket a node transition along with the node count at the transition.
 */
class NodalBracket {
   public:
    double plusEnergy;
    double minusEnergy;
    int node;

    NodalBracket() : plusEnergy(0.0), minusEnergy(0.0), node(0) {}
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

    Eigenstate() : energy(0.0), number(0), psiTrajectory() {}
    Eigenstate(double energy_, int number_, std::vector<double> psiTrajectory_)
        : energy(energy_), number(number_), psiTrajectory(std::move(psiTrajectory_)) {}
};

int countNodes(const std::vector<double>& psi) {
    int nodeCount = 0;
    for (size_t i = 1; i < psi.size(); ++i) {
        if (psi[i - 1] * psi[i] < 0) {  // Check for a sign change indicating a node.
            ++nodeCount;
        }
    }
    return nodeCount;
}

void trimPsiTrajectory(std::vector<double>& psi) {
    int lastIndex = static_cast<int>(psi.size()) - 1;
    int secondLastIndex = static_cast<int>(psi.size()) - 2;

    double absPsiMinOne = std::abs(psi[lastIndex]);
    double absPsiMinTwo = std::abs(psi[secondLastIndex]);

    while (lastIndex > 0 && absPsiMinOne > absPsiMinTwo) {
        --lastIndex;
        --secondLastIndex;
        absPsiMinOne = absPsiMinTwo;
        absPsiMinTwo = std::abs(psi[secondLastIndex]);
    }

    psi.resize(static_cast<size_t>(lastIndex) + 1);  // Trim the trajectory to remove the divergent tail.
}

void normalizePsiTrajectory(std::vector<double>& psi) {
    double norm = 0.0;
    for (const auto& val : psi) {
        norm += val * val;
    }
    norm = std::sqrt(norm);

    if (norm > 0.0) {
        for (auto& val : psi) {
            val /= norm;  // Normalize the trajectory to have a unit norm.
        }
    }
}

// build a lambda that returns the initial conditions based on the parity of the state number (even or odd).
auto initialConditions = [](int stateNumber) -> std::array<double, 2> {
    if (stateNumber % 2 == 0) {
        return {1, 0.0};  // Even states: start with a positive psi and zero derivative.
    } else {
        return {0.0, 1};  // Odd states: start with zero psi and a positive derivative.
    }
};

std::vector<NodalBracket> findNodalBracketsAtDegree(int degree, double xEnd, double stepSize, double energyMin, double energyStepSize,
                                                    int nodesToFind) {
    std::vector<double> potentialTimesTwo = build2TimesPotentialMesh(degree, xEnd, stepSize);

    // Sweep even and odd parity states independently.
    // Each parity tracks its own node count and previous energy.
    struct ParitySweep {
        int nodeCount = 0;
        double previousEnergy;
        std::array<double, 2> iC;
        std::vector<NodalBracket> brackets;

        ParitySweep(double energyMin, std::array<double, 2> iC_) : previousEnergy(energyMin), iC(iC_) {}
    };

    ParitySweep evenSweep(energyMin, {1.0, 0.0});
    ParitySweep oddSweep(energyMin, {0.0, 1.0});

    int totalFound = 0;
    double currentEnergy = energyMin;

    while (totalFound < nodesToFind) {
        currentEnergy += energyStepSize;

        // Even parity sweep
        {
            std::vector<double> psi = NumerovIntegrate(2.0 * currentEnergy, degree, stepSize, xEnd, evenSweep.iC, potentialTimesTwo);
            if (!psi.empty()) {
                int nodeCount = countNodes(psi);
                if (nodeCount > evenSweep.nodeCount) {
                    evenSweep.brackets.emplace_back(currentEnergy, evenSweep.previousEnergy, evenSweep.brackets.size() * 2);
                    ++evenSweep.nodeCount;
                    ++totalFound;
                }
                evenSweep.previousEnergy = currentEnergy;
            }
        }

        // Odd parity sweep
        {
            std::vector<double> psi = NumerovIntegrate(2.0 * currentEnergy, degree, stepSize, xEnd, oddSweep.iC, potentialTimesTwo);
            if (!psi.empty()) {
                int nodeCount = countNodes(psi);
                if (nodeCount > oddSweep.nodeCount) {
                    oddSweep.brackets.emplace_back(currentEnergy, oddSweep.previousEnergy, oddSweep.brackets.size() * 2 + 1);
                    ++oddSweep.nodeCount;
                    ++totalFound;
                }
                oddSweep.previousEnergy = currentEnergy;
            }
        }
    }

    // Merge even and odd brackets sorted by minusEnergy (the lower bracket energy).
    std::vector<NodalBracket> brackets;
    brackets.insert(brackets.end(), evenSweep.brackets.begin(), evenSweep.brackets.end());
    brackets.insert(brackets.end(), oddSweep.brackets.begin(), oddSweep.brackets.end());
    std::sort(brackets.begin(), brackets.end(), [](const NodalBracket& a, const NodalBracket& b) { return a.minusEnergy < b.minusEnergy; });

    brackets.resize(static_cast<size_t>(nodesToFind));
    return brackets;
}

// std::vector<NodalBracket> findNodalBracketsAtDegree(int degree, double xEnd, double stepSize, double energyMin, double energyStepSize,
//                                                     int nodesToFind) {
//     std::vector<NodalBracket> brackets(nodesToFind);
//     std::vector<double> potentialTimesTwo = build2TimesPotentialMesh(degree, xEnd, stepSize);

//     int currentNodeCount = 0;
//     double currentEnergy = energyMin + energyStepSize;  // Start at the first energy step above the minimum.
//     double previousEnergy = energyMin;                  // Initialize previousEnergy to one step below the minimum energy.

//     while (currentNodeCount < nodesToFind) {
//         currentEnergy = previousEnergy + energyStepSize;

//         std::vector<double> psiResults =
//             NumerovIntegrate(2.0 * currentEnergy, degree, stepSize, xEnd, initialConditions(currentNodeCount), potentialTimesTwo);

//         if (psiResults.empty()) {
//             previousEnergy = currentEnergy;  // If the solver failed, skip to the next energy step without updating the node count.
//             continue;
//         }

//         int nodeCount = countNodes(psiResults);

//         if (nodeCount > currentNodeCount) {  // A node transition has occurred between previousEnergy and currentEnergy.
//             brackets[currentNodeCount] = NodalBracket(currentEnergy, previousEnergy, currentNodeCount);
//             ++currentNodeCount;  // Move on to finding the next node transition.
//         }
//         previousEnergy = currentEnergy;  // Update previousEnergy for the next iteration.
//     }
//     return brackets;
// }

std::vector<Eigenstate> findEigenstatesAtDegree(int degree, double xEnd, double stepSize, double convergenceTol, int maxItterations,
                                                const std::vector<NodalBracket>& brackets) {
    std::vector<Eigenstate> eigenstates;
    std::vector<double> potentialTimesTwo = build2TimesPotentialMesh(degree, xEnd, stepSize);

    for (size_t i = 0; i < brackets.size(); ++i) {
        double lowerEnergy = brackets[i].minusEnergy;
        double upperEnergy = brackets[i].plusEnergy;
        double midEnergy = 0.5 * (lowerEnergy + upperEnergy);
        int nodeCount = 0;
        int node = brackets[i].node;
        std::array<double, 2> iC = ::initialConditions(node);  // Get initial conditions based on the parity of the state number.

        double psiAtUpperBoundary = NumerovIntegrate(2.0 * upperEnergy, degree, stepSize, xEnd, iC, potentialTimesTwo)
                                        .back();  // Compute psi at the upper energy boundary for the bisection method.

        std::vector<double> psiResults;
        for (int itteration = 0; itteration < maxItterations; ++itteration) {
        
            psiResults = NumerovIntegrate(2.0 * midEnergy, degree, stepSize, xEnd, iC, potentialTimesTwo);

            if (psiResults.empty()) {
                std::cerr << "Numerov solver failed for energy bracket [" << lowerEnergy << ", " << upperEnergy << "]" << std::endl;
                break;  // If the solver failed, exit the iteration loop and move on to the next bracket.
            }

            nodeCount = countNodes(psiResults);
            double psiAtBoundary = psiResults.back();
            if (psiAtBoundary * psiAtUpperBoundary > 0) {
                upperEnergy = midEnergy;
            } else {
                lowerEnergy = midEnergy;
            }

            midEnergy = 0.5 * (lowerEnergy + upperEnergy);  // Update midEnergy for the next iteration.

            if (std::abs(upperEnergy - lowerEnergy) < convergenceTol) {  // Check for convergence based on the energy bracket width.
                break;
            }
        }

        psiResults = NumerovIntegrate(2.0 * midEnergy, degree, stepSize, xEnd, iC, potentialTimesTwo);  // Final integration at the converged energy.

        if (psiResults.empty()) {
            std::cerr << "Numerov solver failed for final energy " << midEnergy << std::endl;
            continue;  // If the solver failed, skip adding this eigenstate and move on to the next one.
        }

        trimPsiTrajectory(psiResults);       // Trim the psi trajectory to remove any divergent tails.
        normalizePsiTrajectory(psiResults);  // Normalize the psi trajectory to have a unit norm.
        eigenstates.emplace_back(Eigenstate(midEnergy, node, psiResults));
    }
    return eigenstates;
}

class Sweep {
   public:
    int maxDegree;
    int nodesToFind;
    double xEnd = 50.0;

    Sweep(int maxDegree_, int nodesToFind_) : maxDegree(maxDegree_), nodesToFind(nodesToFind_) {}

    std::vector<std::pair<int, std::vector<Eigenstate>>> performSweep(double stepSize, double energyStepSize, double convergenceTol,
                                                                      int maxItterations, double energyMin = 0.001) {
        std::vector<std::pair<int, std::vector<Eigenstate>>> sweepResults(static_cast<size_t>(
            maxDegree / 2));  // Vector to store the results of the sweep, with each entry containing the degree and its corresponding eigenstates.

#pragma omp parallel for
        for (int degree = 2; degree <= maxDegree; degree += 2) {
            std::vector<NodalBracket> brackets = findNodalBracketsAtDegree(degree, xEnd, stepSize, energyMin, energyStepSize, nodesToFind);
            std::vector<Eigenstate> eigenstates = findEigenstatesAtDegree(degree, xEnd, stepSize, convergenceTol, maxItterations, brackets);
            sweepResults[(degree / 2) - 1] =
                std::make_pair(degree, std::move(eigenstates));  // Store the degree and its corresponding eigenstates in the results vector.
        }
        return sweepResults;
    }
};

std::vector<double> resamplePsiTrajectory(const std::vector<double>& psi, double stepSize, int targetPoints, double targetXEnd) {
    double originalXEnd = static_cast<double>(psi.size() - 1) * stepSize;

    std::vector<double> resampled(static_cast<size_t>(targetPoints));
    for (int i = 0; i < targetPoints; ++i) {
        double x = targetXEnd * static_cast<double>(i) / static_cast<double>(targetPoints - 1);

        if (x > originalXEnd) {
            resampled[static_cast<size_t>(i)] = 0.0;
            continue;
        }

        double idx = x / stepSize;
        int lower = static_cast<int>(idx);
        int upper = lower + 1;

        if (upper >= static_cast<int>(psi.size())) {
            resampled[static_cast<size_t>(i)] = psi.back();
            continue;
        }

        double frac = idx - static_cast<double>(lower);
        resampled[static_cast<size_t>(i)] = (1.0 - frac) * psi[static_cast<size_t>(lower)] + frac * psi[static_cast<size_t>(upper)];
    }
    return resampled;
}

void resampleSweepResults(std::vector<std::pair<int, std::vector<Eigenstate>>>& sweepResults, double stepSize, int targetPoints,
                                               double targetXEnd) {

    for (auto& [degree, eigenstates] : sweepResults) {
        for (auto& eigenstate : eigenstates) {
            eigenstate.psiTrajectory = resamplePsiTrajectory(eigenstate.psiTrajectory, stepSize, targetPoints, targetXEnd);
        }
    }
}

void saveSweepResults(const std::vector<std::pair<int, std::vector<Eigenstate>>>& sweepResults, const std::string& filename) {
    std::vector<size_t> degreesShape = {1};
    double degreesData = static_cast<double>(sweepResults.size());
    cnpy::npz_save(filename, "degrees", &degreesData, degreesShape, "w");  // Save the degrees as a separate array in the NPZ file.

    for (size_t i = 0; i < sweepResults.size(); ++i) {
        int degree = sweepResults[i].first;
        const std::vector<Eigenstate>& eigenstates = sweepResults[i].second;

        std::vector<double> energies(eigenstates.size());
        std::vector<double> psiSizes(eigenstates.size());

        for (size_t j = 0; j < eigenstates.size(); ++j) {
            energies[j] = eigenstates[j].energy;                                     // Extract the energy values for each eigenstate.
            psiSizes[j] = static_cast<double>(eigenstates[j].psiTrajectory.size());  // Store the size of each psi trajectory for later retrieval.
        }

        std::vector<size_t> energiesShape = {energies.size()};
        cnpy::npz_save(filename, "energies_degree_" + std::to_string(degree), energies.data(), energiesShape,
                       "a");  // Append energies to the NPZ file.

        std::vector<size_t> psiSizesShape = {psiSizes.size()};
        cnpy::npz_save(filename, "psi_sizes_degree_" + std::to_string(degree), psiSizes.data(), psiSizesShape,
                       "a");  // Append psi sizes to the NPZ file.

        for (size_t j = 0; j < eigenstates.size(); ++j) {
            std::vector<size_t> psiShape = {eigenstates[j].psiTrajectory.size()};
            cnpy::npz_save(filename, "psi_degree_" + std::to_string(degree) + "_state_" + std::to_string(j), eigenstates[j].psiTrajectory.data(),
                           psiShape, "a");  // Append each psi trajectory to the NPZ file with a unique name.
        }
    }
};

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

#endif  // PROCESSING_H