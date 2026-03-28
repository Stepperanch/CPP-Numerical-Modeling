#include <iostream>
#include <string>

#include "processing.h"

/**
 * @brief Main function to perform the sweep and save results.
 * This function sets up the parameters for the sweep, performs the sweep to find eigenstates,
 * resamples the psi trajectories for output, and saves the results to files.
 *
 * Error estamates: the error can be tuned down to the order of 1e-8 by adjusting the energy step size and convergence tolerance,
 * the number of iterations, will also effect the error but above 100 bisections machine percision will likely be the limiting factor.
 *
 * @return int Exit code of the program.
 *
 * @author Nels Buhrley
 * @date 2024-06
 */

int main() {
    // Sweep parameters
    constexpr int maxDegree = 10;
    constexpr int nodesToFind = 11;

    // Solver parameters
    constexpr double stepSize = 0.00001;        // Spatial step size for Numerov integration
    constexpr double energyStepSize = 0.11;     // Energy step size for nodal bracket search
    constexpr double convergenceTol = 1e-8;     // Bisection convergence tolerance on energy bracket width
    constexpr int maxIterations = 100;          // Maximum bisection iterations per eigenstate
    constexpr double energyMin = 0.001;         // Minimum energy to start the sweep from
    constexpr double xEnd = 15.0;               // Spatial domain extends from 0 to xEnd for integration

    //solving Method
    bool useShootingMethod = false; // Set to false to use matching method instead of shooting method

    // Output resampling parameters
    constexpr int targetPsiPoints = 150;  // Number of points to resample ψ(x) onto for output
    constexpr double targetXEnd = 6.0;    // Resample ψ(x) onto [0, targetXEnd] for output

    // Output dir
    std::string outputFile = "output";

    std::cout << "Starting sweep: degrees 2 to " << maxDegree << ", " << nodesToFind << " eigenstates per degree.\n";

    // Perform the sweep to find eigenstates
    Sweep sweep(maxDegree, nodesToFind, xEnd);
    // The performSweep function will find the eigenstates for each degree and return the results in a vector.
    auto results = sweep.performSweep(stepSize, energyStepSize, convergenceTol, maxIterations, energyMin, useShootingMethod);

    // Resample the psi trajectories for output. This will take the original psi trajectories, which may have a large number of points, and resample them to a smaller number of points for easier analysis and visualization.
    resampleSweepResults(results, stepSize, targetPsiPoints, targetXEnd);

    std::cout << "Sweep complete. Saving results to " << outputFile << "...\n";

    // Save the results to files. This will save both the psi trajectories and energy values to separate files for analysis. The psi trajectories will be saved in NPZ format for efficient storage and retrieval, while the energy values will be saved in CSV format for easy analysis.
    saveResults(results, outputFile, targetXEnd);

    std::cout << "Done.\n";

    return 0;
}
