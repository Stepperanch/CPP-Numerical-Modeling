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
    void setMnaught(float mass) {
        Mnaught = mass;
    }

    /** @brief Sets the characteristic length scale in meters. */
    void setLnaught(float length) {
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
    void setAllConversionFactors(float mass, float length, bool useEvUnits) {
        setMnaught(mass);
        setLnaught(length);
        calculateEnergyConversionFactor(useEvUnits);
        calculateTimeConversionFactor(useEvUnits);
    }

    /** @brief Converts a length from reduced units to meters. */
    inline float convertLengthFromReducedUnits(float length) const {
        return length * Lnaught;
    }
    /** @brief Converts a mass from reduced units to kilograms. */
    inline float convertMassFromReducedUnits(float mass) const {
        return mass * Mnaught;
    }
    /** @brief Converts an energy from reduced units to SI or eV units. */
    inline float convertEnergyFromReducedUnits(float energy) const {
        return energy * energyConversionFactor;
    }
    /** @brief Converts a time from reduced units to seconds. */
    inline float convertTimeFromReducedUnits(float time) const {
        return time * timeConversionFactor;
    }
};

/**
 * @brief A helper class for performing numerical integration of ordinary differential equations using the Runge-Kutta methods.
 * Provides methods for fixed-step RK4 integration to compute full trajectories, as well as adaptive RK45 integration to
 * compute final states with error control. The RK45 method returns std::nullopt if it fails to converge within the specified
 * tolerances or maximum steps, allowing the caller to handle these cases gracefully.
 *
 */
class Integrator {
   public:
    /** @brief Performs a single RK4 step.
     * @param x The current value of the independent variable.
     * @param y The current state vector (dependent variables).
     * @param derivFunc A function that computes the derivatives given x and y.
     * @param h The step size.
     *
     * @note This method updates y in place to the new state after taking the RK4 step.
     * This method updates y in place to the new state after taking the RK4 step.
     */
    template <typename DerivFunc>
    inline void RK4Step(double x, std::array<double, 2>& y, const DerivFunc& derivFunc, double h) {
        auto k1 = derivFunc(x, y);
        auto k2 = derivFunc(x + h / 2, y + k1 * (h / 2));
        auto k3 = derivFunc(x + h / 2, y + k2 * (h / 2));
        auto k4 = derivFunc(x + h, y + k3 * h);

        y = y + (k1 + k2 * 2.0 + k3 * 2.0 + k4) * (h / 6.0);
    };

    /**
     * @brief Integrates an ODE using fixed-step RK4 to compute the full trajectory.
     * @param x0 The initial value of the independent variable.
     * @param y0 The initial state vector (dependent variables).
     * @param xEnd The final value of the independent variable to integrate to.
     * @param numSteps The number of steps to take (determines resolution).
     * @param derivFunc A function that computes the derivatives given x and y.
     * @param results A vector to store the resulting trajectory. It will be resized to num
     * @return The full trajectory from x0 to xEnd, with numSteps + 1 points (including the initial state).
     */
    template <typename DerivFunc>
    void RK4IntegrateTrajectory(double x0, std::array<double, 2> y0, double xEnd, int numSteps, const DerivFunc& derivFunc,
                                std::vector<std::array<double, 2>>& results) {
        const double h = (xEnd - x0) / numSteps;
        std::array<double, 2> y = y0;

        results.resize(numSteps + 1);
        results[0] = y;
        for (int i = 0; i < numSteps; ++i) {
            const double x = x0 + i * h;
            RK4Step(x, y, derivFunc, h);
            results[i + 1] = y;
        }
    };

    /**
     * @brief Integrates an ODE using adaptive RK45 to compute the final state at xEnd.
     * @param x0 The initial value of the independent variable.
     * @param y0 The initial state vector (dependent variables).
     * @param xEnd The final value of the independent variable to integrate to.
     * @param h0 The initial step size to use for the integration.
     * @param derivFunc A function that computes the derivatives given x and y.
     *
     * @param absTol The absolute tolerance for error control (default 1e-10).
     * @param relTol The relative tolerance for error control (default 1e-8).
     * @param maxSteps The maximum number of steps to take before giving up (default 1,000,000).
     * @return The final state at xEnd or at divergence if successful, or std::nullopt if the method fails to converge/ diverge within the specified
     * tolerances or maximum steps.
     */
    template <typename DerivFunc>
    std::optional<std::array<double, 2>> RK45IntegrateFinal(double x0, std::array<double, 2> y0, double xEnd, double h0, const DerivFunc& derivFunc,
                                                            double absTol = 1e-10, double relTol = 1e-8, int maxSteps = 1000000) {
        double x = x0;
        std::array<double, 2> y = y0;
        double h = h0;

        // Divergence cap: if |ψ| grows beyond this multiple of its initial value,
        // the solution is clearly diverging. Return early so the caller can read
        // the sign for bisection. 1e4 is conservative enough to avoid false positives
        // while bailing out far sooner than the old 1e8 threshold.
        const double divergenceCap = std::max(1e4 * std::abs(y[0]), 1e4);

        for (int stepCount = 0; stepCount < maxSteps && x < xEnd; ++stepCount) {
            if (x + h > xEnd)
                h = xEnd - x;

            const auto k1 = derivFunc(x, y);
            const auto k2 = derivFunc(x + h * (1.0 / 5.0), y + k1 * (h * (1.0 / 5.0)));
            const auto k3 = derivFunc(x + h * (3.0 / 10.0), y + k1 * (h * (3.0 / 40.0)) + k2 * (h * (9.0 / 40.0)));
            const auto k4 = derivFunc(x + h * (4.0 / 5.0), y + k1 * (h * (44.0 / 45.0)) + k2 * (h * (-56.0 / 15.0)) + k3 * (h * (32.0 / 9.0)));
            const auto k5 = derivFunc(x + h * (8.0 / 9.0), y + k1 * (h * (19372.0 / 6561.0)) + k2 * (h * (-25360.0 / 2187.0)) +
                                                               k3 * (h * (64448.0 / 6561.0)) + k4 * (h * (-212.0 / 729.0)));
            const auto k6 = derivFunc(x + h, y + k1 * (h * (9017.0 / 3168.0)) + k2 * (h * (-355.0 / 33.0)) + k3 * (h * (46732.0 / 5247.0)) +
                                                 k4 * (h * (49.0 / 176.0)) + k5 * (h * (-5103.0 / 18656.0)));
            const auto k7 = derivFunc(x + h, y + k1 * (h * (35.0 / 384.0)) + k3 * (h * (500.0 / 1113.0)) + k4 * (h * (125.0 / 192.0)) +
                                                 k5 * (h * (-2187.0 / 6784.0)) + k6 * (h * (11.0 / 84.0)));

            const auto y5 = y + k1 * (h * (35.0 / 384.0)) + k3 * (h * (500.0 / 1113.0)) + k4 * (h * (125.0 / 192.0)) + k5 * (h * (-2187.0 / 6784.0)) +
                            k6 * (h * (11.0 / 84.0));

            const auto y4 = y + k1 * (h * (5179.0 / 57600.0)) + k3 * (h * (7571.0 / 16695.0)) + k4 * (h * (393.0 / 640.0)) +
                            k5 * (h * (-92097.0 / 339200.0)) + k6 * (h * (187.0 / 2100.0)) + k7 * (h * (1.0 / 40.0));

            double errNorm = 0.0;
            for (size_t i = 0; i < 2; ++i) {
                const double scale = absTol + relTol * std::max(std::abs(y[i]), std::abs(y5[i]));
                const double err = std::abs(y5[i] - y4[i]) / scale;
                errNorm = std::max(errNorm, err);
            }

            if (errNorm <= 1.0) {
                x += h;
                y = y5;
                if (std::abs(y[0]) > divergenceCap)
                    return y;  // clearly diverging — return now so caller can read the sign
            }

            if (h < 1e-15 || std::isnan(y[0]) || std::isinf(y[0])) {
                return std::nullopt;
            }

            h *= std::clamp(0.9 * std::pow(std::max(errNorm, 1e-12), -0.2), 0.2, 5.0);
            ;
        }

        if (x < xEnd)
            return std::nullopt;  // maxSteps exhausted

        return y;
    }
};

/** @brief A system for testing quantum mechanical calculations.
 * The potential is V(x) = x^n / n, and the Schrodinger equation is solved for a given energy level E using both adaptive RK45 (for final state) and
 * fixed-step RK4 (for full trajectory).
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

    /** @brief Solves the Schrodinger equation for the given initial conditions and parameters.
     * @param x0 The initial position.
     * @param Y0 The initial state vector.
     * @param xEnd The final position.
     * @param h0 The initial step size.
     * @return The final state at xEnd or at divergence if successful, or std::nullopt if the method fails to converge/ diverge within the specified
     * tolerances or maximum steps.
     */
    std::optional<std::array<double, 2>> solve(double x0, std::array<double, 2> Y0, double xEnd, double h0 = 0.01) const {
        Integrator integrator;
        return integrator.RK45IntegrateFinal(x0, Y0, xEnd, h0,
                                             [this](double x, const std::array<double, 2>& Yvec) { return derivativeFunc(x, Yvec); });
    }

    /** @brief Computes the full trajectory using fixed-step RK4.
     * @param x0 The initial position.
     * @param Y0 The initial state vector.
     * @param xEnd The final position.
     * @param numSteps The number of steps to take.
     * @param results A vector to store the trajectory points.
     */
    void solve(double x0, std::array<double, 2> Y0, double xEnd, int numSteps, std::vector<std::array<double, 2>>& results) const {
        Integrator integrator;
        integrator.RK4IntegrateTrajectory(
            x0, Y0, xEnd, numSteps, [this](double x, const std::array<double, 2>& Yvec) { return derivativeFunc(x, Yvec); }, results);
    }

    /** @brief Returns only psi(xEnd) using adaptive RK45.
     * @param x0 The initial position.
     * @param Y0 The initial state vector [psi, dpsi/dx].
     * @param xEnd The final position.
     * @param h0 Initial RK45 step size hint.
     * @return psi(xEnd) (or divergence psi) if successful, std::nullopt on solver failure.
     */
    std::optional<double> solvePsi(double x0, std::array<double, 2> Y0, double xEnd, double h0 = 0.01) const {
        const auto finalState = solve(x0, Y0, xEnd, h0);
        if (!finalState) {
            return std::nullopt;
        }
        return (*finalState)[0];
    }

    /** @brief Returns only the psi trajectory using fixed-step RK4.
     * @param x0 The initial position.
     * @param Y0 The initial state vector [psi, dpsi/dx].
     * @param xEnd The final position.
     * @param numSteps Number of RK4 steps.
     * @param psiResults Output vector containing psi values at each step.
     */
    void solvePsi(double x0, std::array<double, 2> Y0, double xEnd, int numSteps, std::vector<double>& psiResults) const {
        std::vector<std::array<double, 2>> fullResults;
        solve(x0, Y0, xEnd, numSteps, fullResults);

        psiResults.resize(fullResults.size());
        for (size_t i = 0; i < fullResults.size(); ++i) {
            psiResults[i] = fullResults[i][0];
        }
    }
};

class NodalBracket {
   public:
    double plusEnergy;
    double minusEnergy;
    int node;

    NodalBracket(double plusEnergy_, double minusEnergy_, int node_)
        : plusEnergy(plusEnergy_), minusEnergy(minusEnergy_), node(node_) {}
};

class ESweep {
    int n;
    int nodesToFind;  // Number of energy eigenstates to find, determined by counting nodes in the wavefunction
    double E_min;     // Minimum energy level for which we want to solve the Schrodinger equation
    double E_max;     // Maximum energy level for which we want to solve the Schrodinger equation

    ESweep(int n_, double E_min_, double E_max_, int nodesToFind_ = 8) : n(n_), E_min(E_min_), E_max(E_max_), nodesToFind(nodesToFind_) {}

    /**
     * @brief Performs an energy sweep to find the first 8 energy eigenstates of the quantum test system.
     * @param x0 The initial position.
     * @param Y0 The initial state vector.
     * @param xEnd The final position.
     * @param h0 The initial step size.
     * @return A vector of the first 8 energy eigenstates found.
     */
    std::vector<std::optional<std::array<double, 2>>> performSweep(double x0, std::array<double, 2> Y0, double xEnd, double h0 = 0.01) {
        std::vector<std::optional<std::array<double, 2>>> eigenstates;
        return eigenstates;
    }

    /** @brief Counts the number of nodes in a wavefunction.
     * @param state The wavefunction state vector.
     * @return The number of nodes.
     */
    int countNodes(const std::array<double, 2>& state) const {
        int nodes = 0;
        for (size_t i = 1; i < state.size(); ++i) {
            if (state[i - 1] * state[i] < 0) {
                ++nodes;
            }
        }
        return nodes;
    }

    /** @brief Uses bisection to find energy eigenstates by counting nodes in the wavefunction.
     *
     */
    std::vector<NodalBracket> findNodalBrackets() const {
        return {};
    };
};
#endif  // PROCESSING_H