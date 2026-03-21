#include <iostream>
#include <string>

#include "processing.h"

int main() {
    // Sweep parameters
    constexpr int maxDegree = 10;
    constexpr int nodesToFind = 10;

    // Solver parameters
    constexpr double stepSize = 0.00001;         // Spatial step size for Numerov integration
    constexpr double energyStepSize = 0.5;      // Energy step size for nodal bracket search
    constexpr double convergenceTol = 1e-15;    // Bisection convergence tolerance on energy bracket width
    constexpr int maxIterations = 300;          // Maximum bisection iterations per eigenstate
    constexpr double energyMin = 0.001;         // Minimum energy to start the sweep from
    constexpr int targetPsiPoints = 150;        // Number of points to resample ψ(x) onto for output
    constexpr double targetXEnd = 6.0;          // Resample ψ(x) onto [0, targetXEnd] for output

    // Output file
    const std::string outputFile = "output/eigenstates.npz";

    std::cout << "Starting sweep: degrees 2 to " << maxDegree << ", " << nodesToFind << " eigenstates per degree.\n";

    Sweep sweep(maxDegree, nodesToFind);
    auto results = sweep.performSweep(stepSize, energyStepSize, convergenceTol, maxIterations, energyMin);

    resampleSweepResults(results, stepSize, targetPsiPoints, targetXEnd);

    std::cout << "Sweep complete. Saving results to " << outputFile << "...\n";

    saveSweepResults(results, outputFile, targetXEnd);

    std::cout << "Done.\n";


    return 0;
}
