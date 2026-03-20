#include <iostream>
#include <string>

#include "processing.h"

int main() {
    // Sweep parameters
    constexpr int    maxDegree     = 6;
    constexpr int    nodesToFind   = 10;

    // Solver parameters
    constexpr double stepSize      = 0.0001;   // Spatial step size for Numerov integration
    constexpr double energyStepSize = 0.5;  // Energy step size for nodal bracket search
    constexpr double convergenceTol = 1e-15; // Bisection convergence tolerance on energy bracket width
    constexpr int    maxIterations  = 150;  // Maximum bisection iterations per eigenstate
    constexpr double energyMin      = 0.001; // Minimum energy to start the sweep from

    // Output file
    const std::string outputFile = "output/eigenstates.npz";

    std::cout << "Starting sweep: degrees 2 to " << maxDegree
              << ", " << nodesToFind << " eigenstates per degree.\n";

    Sweep sweep(maxDegree, nodesToFind);
    auto results = sweep.performSweep(stepSize, energyStepSize, convergenceTol, maxIterations, energyMin);

    resampleSweepResults(results, stepSize, 50, 4.0);

    std::cout << "Sweep complete. Saving results to " << outputFile << "...\n";

    saveSweepResults(results, outputFile);

    std::cout << "Done.\n";

    // Print a summary of found energies to console
    for (const auto& [degree, eigenstates] : results) {
        std::cout << "Degree " << degree << ":\n";
        for (const auto& state : eigenstates) {
            std::cout << "  State " << state.number
                      << "  E = " << state.energy
                      << "  psi points = " << state.psiTrajectory.size()
                      << "\n";
        }
    }

    return 0;
}

