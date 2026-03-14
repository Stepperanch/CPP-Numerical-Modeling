#include "processing.h"

int main(int argc, char* argv[]) {
    // Example usage of the MolucularSystem class

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    std::map<std::string, std::string> config = parseConfigFile(argv[1]);

    MolucularSystem system(config);
    std::cout << "Running simulation with " << system.numParticles << " particles for " << system.timeSteps << " time steps on " << omp_get_num_threads() << " threads." << std::endl;
    system.runSimulation();
    std::cout << "Simulation complete. Saving results..." << std::endl;

    //system.save();

    system.binSave();  // Save results in binary format for efficient loading in Python

    std::cout << "Results saved to NPZ file." << std::endl;

    return 0;
}