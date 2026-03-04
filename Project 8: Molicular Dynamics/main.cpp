#include "processing.h"

int main() {
    // Example usage of the MolucularSystem class
    std::vector<std::array<double, MolucularSystem::d>> initialPositions;

    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 50; j++) {

            initialPositions.push_back({i * 1.1 + 48 + rand() / RAND_MAX * 0.7 , j * 1.1 + 48 + rand() / RAND_MAX * 0.7});  // Place particles in a grid with spacing of 1.5 sigma
        }
    }


    unsigned int timeSteps = 100000;
    double finalTime = 400.0;
    float boxSize = 150.0;




    MolucularSystem system(initialPositions, buildEnergyFunction(timeSteps), timeSteps, finalTime, boxSize);
    std::cout << "Running simulation with " << system.numParticles << " particles for " << system.timeSteps << " time steps on " << omp_get_num_threads() << " threads." << std::endl;
    system.runSimulation();
    std::cout << "Simulation complete. Saving results..." << std::endl;

    system.save();
    std::cout << "Results saved to NPZ file." << std::endl;

    return 0;
}