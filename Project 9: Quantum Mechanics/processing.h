#ifndef PROCESSING_H
#define PROCESSING_H

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

/**
 * @file processing.h
 * @brief This header file contains the core processing functions for the quantum mechanics project, including potential computation, Numerov integration, node counting, and result saving.
 * The functions defined here are used to compute the potential, perform Numerov integration for both forward and backward directions, count nodes in the wavefunction, resample psi trajectories, and save results to files.
 * The main classes defined in this file include NodalBracket for bracketing energy eigenvalues based on node counting, and Eigenstate for representing energy eigenstates with their associated psi trajectories.
 * The functions are designed to be efficient and numerically stable, with checks for divergence and handling of edge cases in the integration process.
 * The file also includes utility functions for resampling psi trajectories to a target number of points and saving results in both NPZ and CSV formats for analysis.
 * The processing functions are intended to be used in the main program to perform energy sweeps and find eigenstates of the quantum system under study.
 * The code is written in C++17 and makes use of standard library features such as vectors, arrays, file streams, and filesystem operations for handling output directories and files.
 * Overall, this header file serves as the computational backbone for the quantum mechanics project, providing the necessary functions to compute potentials, perform integrations, count nodes, and save results for further analysis and visualization.
 *
 * @author Nels Buhrley
 * @date 2024-06-01
 */


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
 * This integrator is used for the shooting method to find energy eigenstates.
 * @param E The energy level.
 * @param degree The degree of the potential.
 * @param xEnd The final position.
 * @param numSteps Number of uniform intervals.
 * @param Y0 The initial state vector. Y0[0] = psi(x0), Y0[1] = psi'(x0).
 * @param divergenceThreshold The threshold for detecting divergence.
 * @param potential The precomputed potential values at the grid points.
 * @return A vector containing the integrated psi values at each grid point.
 */
std::vector<double> NumerovIntegrate(double ETimesTwo, double stepSize, double xEnd, const std::array<double, 2>& Y0,
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

/** @brief Overloads the NumerovIntegrate function for backward integration.
 * @param ETimesTwo The energy level multiplied by 2.
 * @param degree The degree of the potential.
 * @param stepSize The step size for the integration.
 * @param xEnd The final position.
 * @param potentialTimesTwo A vector containing the precomputed potential values at the grid points, multiplied by 2.
 * @return A vector the posision and derivitave of psi at 0
 */
std::array<double, 2> NumerovIntegrate(double ETimesTwo, double stepSize, double xEnd, const std::vector<double>& potentialTimesTwo) {
    int numSteps = static_cast<int>(xEnd / stepSize);
    stepSize = xEnd / static_cast<double>(numSteps);  // Recalculate step size to ensure it divides xEnd evenly.
    const double dx = stepSize;
    const double h2 = dx * dx;

    std::array<double, 2> Yf = {0.0,
                                1e-50};  // Start with a very small psi value and a positive derivative at the end point to avoid numerical issues.

    /** @brief Lambda to compute g(x_i) = 2(V(x_i) - E) at grid index i.
     * Computes the potential on the fly for backward integration.
     */
    auto gAt = [&](int i) -> double { return potentialTimesTwo[static_cast<size_t>(i)] - ETimesTwo; };

    std::vector<double> psiResults(static_cast<size_t>(numSteps) + 1);  // Vector to store psi values at each grid point

    psiResults[static_cast<size_t>(numSteps)] = Yf[0];  // Set initial psi value at xEnd
    psiResults[static_cast<size_t>(numSteps - 1)] =
        Yf[0] - dx * Yf[1] + 0.5 * h2 * gAt(numSteps) * Yf[0];  // Compute psi at the first step backward using Taylor expansion

    double aPrev;
    double aCurr;
    double aNext;
    double psiNext;

    for (int i = numSteps - 1; i > 0; --i) {
        // Precompute the coefficients for the Numerov formula to improve readability.
        aPrev = 1.0 - (h2 / 12.0) * gAt(i + 1);
        aCurr = 1.0 + (5.0 * h2 / 12.0) * gAt(i);
        aNext = 1.0 - (h2 / 12.0) * gAt(i - 1);

        if (std::abs(aNext) < 1e-14) {
            break;
        }

        psiNext = (2.0 * aCurr * psiResults[static_cast<size_t>(i)] - aPrev * psiResults[static_cast<size_t>(i + 1)]) / aNext;

        psiResults[static_cast<size_t>(i - 1)] = psiNext;
    }
    return {psiResults[0], (psiResults[1] - psiResults[0]) / dx};  // Return the psi value and its derivative at x=0 for matching conditions.
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

/** @brief Count the number of nodes in a wavefunction.
 * @param psi The wavefunction values at each grid point.
 * @return The number of nodes.
 */
int countNodes(const std::vector<double>& psi) {
    int nodeCount = 0;
    for (size_t i = 1; i < psi.size(); ++i) {
        if (psi[i - 1] * psi[i] < 0) {  // Check for a sign change indicating a node.
            ++nodeCount;
        }
    }
    return nodeCount;
}

/** @brief Trim the wavefunction trajectory to remove the divergent tail.
 * @param psi The wavefunction values at each grid point.
 */
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

/** @brief Normalize the wavefunction trajectory to have a unit norm.
 * @param psi The wavefunction values at each grid point.
 */
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
/** @brief Returns the initial conditions for the wavefunction based on its parity.
 * @param stateNumber The number of the state (even or odd).
 * @return An array containing the initial wavefunction value and its derivative.
 */
auto initialConditions = [](int stateNumber) -> std::array<double, 2> {
    if (stateNumber % 2 == 0) {
        return {1, 0.0};  // Even states: start with a positive psi and zero derivative.
    } else {
        return {0.0, 1};  // Odd states: start with zero psi and a positive derivative.
    }
};

/** @brief Find nodal brackets for a given degree.
 * this function performs a sweep over energy values, integrating the wavefunction for both even and odd parity states,
 * counting nodes, and collecting brackets where node transitions occur.
 * @param degree The degree of the potential.
 * @param xEnd The end of the spatial domain.
 * @param stepSize The spatial step size.
 * @param energyMin The minimum energy to start the sweep from.
 * @param energyStepSize The energy step size for the sweep.
 * @param nodesToFind The number of nodes to find.
 * @return A vector of nodal brackets.
 */
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
            std::vector<double> psi = NumerovIntegrate(2.0 * currentEnergy, stepSize, xEnd, evenSweep.iC, potentialTimesTwo);
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
            std::vector<double> psi = NumerovIntegrate(2.0 * currentEnergy, stepSize, xEnd, oddSweep.iC, potentialTimesTwo);
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

/** @brief Find eigenstates for a given degree using the shooting method.
 * This function takes the nodal brackets found for the given degree and performs a bisection method to find the energy eigenvalues corresponding to those brackets.
 * For each bracket, it integrates the wavefunction at the midpoint energy and checks the sign of psi at the boundary to determine which side of the bracket to keep, iterating until convergence.
 * @param degree The degree of the potential.
 * @param xEnd The end of the spatial domain.
 * @param stepSize The spatial step size.
 * @param convergenceTol The convergence tolerance for the bisection method.
 * @param maxItterations The maximum number of iterations for the bisection method.
 * @param brackets The nodal brackets found for the given degree.
 * @return A vector of eigenstates.
 */
std::vector<Eigenstate> findEigenstatesAtDegreeShooting(int degree, double xEnd, double stepSize, double convergenceTol, int maxItterations,
                                                        const std::vector<NodalBracket>& brackets) {
    std::vector<Eigenstate> eigenstates;
    std::vector<double> potentialTimesTwo = build2TimesPotentialMesh(degree, xEnd, stepSize);
    std::vector<double> psiResults;
    for (size_t i = 0; i < brackets.size(); ++i) {
        double lowerEnergy = brackets[i].minusEnergy;
        double upperEnergy = brackets[i].plusEnergy;
        double midEnergy = 0.5 * (lowerEnergy + upperEnergy);
        int nodeCount = 0;
        int node = brackets[i].node;
        std::array<double, 2> iC = ::initialConditions(node);  // Get initial conditions based on the parity of the state number.

        double psiAtUpperBoundary = NumerovIntegrate(2.0 * upperEnergy, stepSize, xEnd, iC, potentialTimesTwo)
                                        .back();  // Compute psi at the upper energy boundary for the bisection method.

        for (int itteration = 0; itteration < maxItterations; ++itteration) {
            psiResults = NumerovIntegrate(2.0 * midEnergy, stepSize, xEnd, iC, potentialTimesTwo);

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

        psiResults = NumerovIntegrate(2.0 * midEnergy, stepSize, xEnd, iC, potentialTimesTwo);  // Final integration at the converged energy.

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

/** @brief Find eigenstates for a given degree using the matching method.
 * This function takes the nodal brackets found for the given degree and performs a bisection method to find the energy eigenvalues corresponding to those brackets.
 * For each bracket, it integrates the wavefunction at the midpoint energy and checks the sign of psi or its derivative at the boundary (depending on the parity of the state) to determine which side of the bracket to keep, iterating until convergence.
 * @param degree The degree of the potential.
 * @param xEnd The end of the spatial domain.
 * @param stepSize The spatial step size.
 * @param convergenceTol The convergence tolerance for the bisection method.
 * @param maxItterations The maximum number of iterations for the bisection method.
 * @param brackets The nodal brackets found for the given degree.
 * @return A vector of eigenstates.
 */
std::vector<Eigenstate> findEigenstatesAtDegreeMatching(int degree, double xEnd, double stepSize, double convergenceTol, int maxItterations,
                                                        const std::vector<NodalBracket>& brackets) {
    std::vector<Eigenstate> eigenstates;
    std::vector<double> potentialTimesTwo = build2TimesPotentialMesh(degree, xEnd, stepSize);

    std::array<double, 2> psiResults;
    for (size_t i = 0; i < brackets.size(); ++i) {
        double lowerEnergy = brackets[i].minusEnergy;
        std::array<double, 2> psiAtLowerBoundary = NumerovIntegrate(2.0 * lowerEnergy, stepSize, xEnd, potentialTimesTwo);
        // Compute psi and its derivative at the lower energy boundary for matching conditions.

        double upperEnergy = brackets[i].plusEnergy;
        std::array<double, 2> psiAtUpperBoundary = NumerovIntegrate(2.0 * upperEnergy, stepSize, xEnd, potentialTimesTwo);
        // Compute psi and its derivative at the upper energy boundary for matching conditions.

        double midEnergy = 0.5 * (lowerEnergy + upperEnergy);
        int node = brackets[i].node;
        std::array<double, 2> mC = ::initialConditions(node);  // Get matching conditions based on the parity of the state number.

        for (int itteration = 0; itteration < maxItterations; ++itteration) {
            psiResults = NumerovIntegrate(2.0 * midEnergy, stepSize, xEnd,
                                          potentialTimesTwo);  // Perform backward integration to get psi and its derivative at x=0.
            if (psiResults.empty()) {
                std::cerr << "Numerov solver failed for energy bracket [" << lowerEnergy << ", " << upperEnergy << "]" << std::endl;
                break;  // If the solver failed, exit the iteration loop and move on to the next bracket.
            }

            // if even parity minimise psi value at x=0, if odd parity minimise derivativ at x=0.
            double midValue = (node % 2 == 1) ? psiResults[0] : psiResults[1];  // Select the appropriate matching value based on parity.
            double upperBoundaryValue = (node % 2 == 1) ? psiAtUpperBoundary[0]
                                                   : psiAtUpperBoundary[1];  // Select the corresponding value from the lower boundary for comparison.
            double lowerBoundaryValue = (node % 2 == 1) ? psiAtLowerBoundary[0]
                                                   : psiAtLowerBoundary[1];  // Select the corresponding value from the upper boundary for comparison.

        // std::cout << "Degree: " << degree << ", Node: " << node << ", Iteration: " << itteration << ", Mid Value: " << midValue
        //               << ", Lower Boundary Value: " << lowerBoundaryValue << ", Upper Boundary Value: " << upperBoundaryValue
        //               << std::endl;
            if (midValue * lowerBoundaryValue < 0) {
                upperEnergy = midEnergy;
            } else {
                lowerEnergy = midEnergy;
            }

            midEnergy = 0.5 * (lowerEnergy + upperEnergy);  // Update midEnergy for the next iteration.

            // std::cout << "Degree: " << degree << ", Node: " << node << ", Iteration: " << itteration << ", Energy Bracket: [" << lowerEnergy
            //           << ", " << upperEnergy << "], Mid Energy: " << midEnergy << std::endl;

            if (std::abs(upperEnergy - lowerEnergy) < convergenceTol) {  // Check for convergence based on the energy bracket width.
                break;
            }
        }
        std::vector<double> psiFinalTrajectory =
            NumerovIntegrate(2.0 * midEnergy, stepSize, xEnd, mC, potentialTimesTwo);  // Final integration at the converged energy.

        if (psiFinalTrajectory.empty()) {
            std::cerr << "Numerov solver failed for final energy " << midEnergy << std::endl;
            continue;  // If the solver failed, skip adding this eigenstate and move on to the next one.
        }
        trimPsiTrajectory(psiFinalTrajectory);       // Trim the psi trajectory to remove any divergent tails.
        normalizePsiTrajectory(psiFinalTrajectory);  // Normalize the psi trajectory to have a unit norm.
        eigenstates.emplace_back(Eigenstate(midEnergy, node, psiFinalTrajectory));
    }
    return eigenstates;
}

/** @brief Finds eigenstates for a given degree using the bisection method.
 * This function serves as a wrapper that allows the user to choose between the shooting method and the matching method for finding eigenstates based on the provided nodal brackets.
 * @param degree The degree of the potential.
 * @param xEnd The end of the spatial domain.
 * @param stepSize The spatial step size.
 * @param convergenceTol The convergence tolerance for the bisection method.
 * @param maxItterations The maximum number of iterations for the bisection method.
 * @param brackets The nodal brackets found for the given degree.
 * @param useShooting A boolean flag to choose between the shooting method (true) and the matching method (false) for finding eigenstates.
 * @return A vector of eigenstates found for the given degree.
*/
std::vector<Eigenstate> findEigenstatesAtDegree(int degree, double xEnd, double stepSize, double convergenceTol, int maxItterations,
                                                const std::vector<NodalBracket>& brackets, bool useShooting = true) {
    if (useShooting) {
        return findEigenstatesAtDegreeShooting(degree, xEnd, stepSize, convergenceTol, maxItterations, brackets);
    } else {
        return findEigenstatesAtDegreeMatching(degree, xEnd, stepSize, convergenceTol, maxItterations, brackets);
    }
}

/** @brief A class for performing a sweep of eigenstates across different degrees.
 * This class allows for the systematic calculation of eigenstates for a range of potential degrees.
 */
class Sweep {
   public:
    int maxDegree;
    int nodesToFind;
    double xEnd;

    Sweep(int maxDegree_, int nodesToFind_, double xEnd_) : maxDegree(maxDegree_), nodesToFind(nodesToFind_), xEnd(xEnd_) {}

    std::vector<std::pair<int, std::vector<Eigenstate>>> performSweep(double stepSize, double energyStepSize, double convergenceTol,
                                                                      int maxItterations, double energyMin = 0.001, bool useShooting = true) {
        std::vector<std::pair<int, std::vector<Eigenstate>>> sweepResults(static_cast<size_t>(
            maxDegree / 2));  // Vector to store the results of the sweep, with each entry containing the degree and its corresponding eigenstates.

#pragma omp parallel for
        for (int degree = 2; degree <= maxDegree; degree += 2) {
            std::vector<NodalBracket> brackets = findNodalBracketsAtDegree(degree, xEnd, stepSize, energyMin, energyStepSize, nodesToFind);
            std::vector<Eigenstate> eigenstates =
                findEigenstatesAtDegree(degree, xEnd, stepSize, convergenceTol, maxItterations, brackets, useShooting);
            sweepResults[(degree / 2) - 1] =
                std::make_pair(degree, std::move(eigenstates));  // Store the degree and its corresponding eigenstates in the results vector.
        }
        return sweepResults;
    }
};

/** @brief Resamples a psi trajectory to a target number of points.
 * This function takes a psi trajectory and resamples it to a target number of points, scaling the x-axis accordingly.
 * @param psi The original psi trajectory.
 * @param stepSize The spatial step size of the original trajectory.
 * @param targetPoints The number of points to resample the trajectory to.
 * @param targetXEnd The end of the spatial domain for the resampled trajectory.
 * @return A vector containing the resampled psi trajectory.
 */
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

/** @brief Resamples the psi trajectories in the sweep results to a target number of points.
 * This function takes the sweep results and resamples each psi trajectory to a target number of points, scaling the x-axis accordingly.
 * @param sweepResults The vector of sweep results containing eigenstates.
 * @param stepSize The spatial step size of the original trajectories.
 * @param targetPoints The number of points to resample the trajectories to.
 * @param targetXEnd The end of the spatial domain for the resampled trajectories.
 */
void resampleSweepResults(std::vector<std::pair<int, std::vector<Eigenstate>>>& sweepResults, double stepSize, int targetPoints, double targetXEnd) {
    for (auto& [degree, eigenstates] : sweepResults) {
        for (auto& eigenstate : eigenstates) {
            eigenstate.psiTrajectory = resamplePsiTrajectory(eigenstate.psiTrajectory, stepSize, targetPoints, targetXEnd);
        }
    }
}

/** @brief Saves the psi results to an NPZ file.
 * This function takes the sweep results and saves the psi trajectories to an NPZ file, along with associated metadata.
 * @param sweepResults The vector of sweep results containing eigenstates.
 * @param filename The name of the NPZ file to save the results to.
 * @param targetXEnd The end of the spatial domain for the resampled trajectories.
 */
void savePsiResultsNPZ(const std::vector<std::pair<int, std::vector<Eigenstate>>>& sweepResults, const std::string& filename,
                      double targetXEnd = 8.0) {
    std::cout << "Saving psi results to NPZ file: " << filename << std::endl;
    std::vector<size_t> degreesShape = {1};
    double degreesData = static_cast<double>(sweepResults.size());
    cnpy::npz_save(filename, "degrees", &degreesData, degreesShape, "w");  // Save the degrees as a separate array in the NPZ file.

    // Save targetXEnd for x-axis scaling in plots
    std::vector<size_t> targetXEndShape = {1};
    std::vector<double> targetXEndData = {targetXEnd};
    cnpy::npz_save(filename, "targetXEnd", targetXEndData.data(), targetXEndShape, "a");

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

/** @brief Saves the energy results to a CSV file.
 * This function takes the sweep results and saves the energy values to a CSV file.
 * @param sweepResults The vector of sweep results containing eigenstates.
 * @param filename The name of the CSV file to save the results to.
 */
void saveEnergyResultsCSV(const std::vector<std::pair<int, std::vector<Eigenstate>>>& sweepResults, const std::string& filename) {

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    std::cout << "Saving energy results to CSV file: " << filename << std::endl;
    outFile << "Degree,State Number,Energy\n";  // Write CSV header
    for (const auto& [degree, eigenstates] : sweepResults) {
        for (const auto& eigenstate : eigenstates) {
            outFile << degree << "," << eigenstate.number << "," << eigenstate.energy << "\n";  // Write degree, state number, and energy for each eigenstate.
        }
    }
}

/**
 * @brief Saves the results of the sweep to files.
 * This function saves both the psi trajectories and energy values to separate files for analysis.
 * @param sweepResults The vector of sweep results containing eigenstates.
 * @param outputDir The directory where the results will be saved.
 * @param targetXEnd The end of the spatial domain for the resampled trajectories.
 */
void saveResults(const std::vector<std::pair<int, std::vector<Eigenstate>>>& sweepResults, const std::string& outputDir, double targetXEnd = 8.0) {
    std::filesystem::create_directories(outputDir);  // Ensure the output directory exists.
    savePsiResultsNPZ(sweepResults, outputDir + "/eigenstates.npz", targetXEnd);  // Save psi results in NPZ format for efficient storage and retrieval.
    saveEnergyResultsCSV(sweepResults, outputDir + "/energy_results.csv");  // Save energy results in CSV format for easy analysis and plotting.
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

#endif  // PROCESSING_Hs