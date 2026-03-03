#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#define _USE_MATH_DEFINES

#include <omp.h>

#include "cnpy.h"

#ifndef PROCESSING_H
#define PROCESSING_H

class MolucularSystem {
   public:
    static constexpr int d = 2;
    static constexpr float rc = 2.5;       // Cutoff distance for Lennard-Jones potential in units of sigma
    static constexpr float rc2 = rc * rc;  // Square of the cutoff distance for efficiency

    float L;  // Size of the box in units of sigma (assuming a box for simplicity)

    std::vector<std::array<double, d>> positions;
    std::vector<std::array<double, d>> velocities;
    std::vector<std::array<double, d>> accelerations;

    double currentPE;         // Current potential energy of the system
    double currentKE_times2;  // 2 x Current kinetic energy of the system
    double currentTE;         // Current total energy of the system

    std::vector<double> temperatures;
    std::vector<double> potentialEnergies;
    std::vector<double> kineticEnergies;
    std::vector<double> totalEnergies;

    std::vector<std::pair<int, double>> energyFunction;  // Stores the timesteps and the ∆E values for the system
    unsigned int energyFunctionIndex = 0;                // Index to track the current position in the energyFunction vector

    unsigned int numParticles;
    unsigned int timeSteps;
    double finalTime;
    double timeStep;

    // Assume sigma, epsilon, and mass are all 1 for simplicity in reduced units

    MolucularSystem(std::vector<std::array<double, d>> initialPositions, std::vector<std::pair<int, double>> energyFunction, unsigned int timeSteps,
                    double finalTime, float boxSize) {
        this->numParticles = initialPositions.size();
        this->timeSteps = timeSteps;
        this->finalTime = finalTime;
        this->L = boxSize;
        this->energyFunction = energyFunction;
        positions.resize(numParticles * timeSteps);
        accelerations.resize(numParticles);
        velocities.resize(numParticles);
        temperatures.resize(timeSteps);
        potentialEnergies.resize(timeSteps);
        kineticEnergies.resize(timeSteps);
        totalEnergies.resize(timeSteps);

        // give partical 0 a small velocity to break symmetry and allow the system to evolve

        // #pragma omp parallel for schedule(static)
        for (unsigned int i = 0; i < numParticles; i++) {
            positions[i] = initialPositions[i];  // Set initial positions for the first time step
            velocities[i] = {0.0, 0.0};          // Initialize velocities to zero
            accelerations[i] = {0.0, 0.0};       // Initialize accelerations to zero
        }
        timeStep = finalTime / timeSteps;

        velocities[0] = {0.001, 0.001};  // Small initial velocity for particle 0
    }

    inline std::array<double, d>& getPosition(unsigned int timeStep, unsigned int particleIndex) {
        return positions[timeStep * numParticles + particleIndex];
    }

    inline void setPosition(unsigned int timeStep, unsigned int particleIndex, const std::array<double, d>& newPosition) {
        positions[timeStep * numParticles + particleIndex] = newPosition;
    }

    void calculateAccelerations(unsigned int t) {
        // Implement the logic to calculate accelerations based on the current positions using the leanord jonse potential
        currentPE = 0.0;  // Reset potential energy before calculation

        // #pragma omp parallel for schedule(static)
        for (unsigned int i = 0; i < numParticles; i++) {
            accelerations[i] = {0.0, 0.0};  // Reset accelerations before calculation
        }

#pragma omp parallel if (numParticles > 50)  // Only parallelize if there are enough particles to justify the overhead
        {
            std::vector<std::array<double, d>> localAccelerations(numParticles, {0.0, 0.0});  // Thread-local storage for accelerations
            double localPE = 0.0;                                                             // Thread-local storage for potential energy
#pragma omp for schedule(guided)
            for (int p1_ind = 0; p1_ind < numParticles; ++p1_ind) {
                std::array<double, d>& p1_pos = getPosition(t, p1_ind);

                for (int p2_ind = p1_ind + 1; p2_ind < numParticles; p2_ind++) {
                    std::array<double, d>& p2_pos = getPosition(t, p2_ind);

                    double dx = p1_pos[0] - p2_pos[0];
                    double dy = p1_pos[1] - p2_pos[1];
                    // double dz = p1_pos[2] - p2_pos[2]; // For 3D

                    // Apply minimum image convention for periodic boundary conditions
                    dx = dx - L * std::round(dx / L);
                    dy = dy - L * std::round(dy / L);

                    double r2 = dx * dx + dy * dy;  // + dz*dz for 3D

                    if (r2 > rc2)
                        continue;  // Skip if beyond cutoff distance

                    double one_over_r2 = 1.0 / r2;
                    double one_over_r6 = one_over_r2 * one_over_r2 * one_over_r2;
                    double one_over_r12 = one_over_r6 * one_over_r6;
                    double r_times_force_magnitude = 24.0 * (2 * one_over_r12 - one_over_r6);  // Lennard-Jones force

                    localPE += 4.0 * (one_over_r12 - one_over_r6);  // Lennard-Jones potential energy contribution

                    double ax = r_times_force_magnitude * dx / r2;
                    double ay = r_times_force_magnitude * dy / r2;
                    // double fz = force_magnitude * dz / r; // For 3D

                    localAccelerations[p1_ind][0] += ax;  // Update acceleration for particle 1
                    localAccelerations[p1_ind][1] += ay;
                    // accelerations[p1_ind][2] += fz; // For 3D

                    localAccelerations[p2_ind][0] -= ax;  // Update acceleration for particle 2 (Newton's third law)
                    localAccelerations[p2_ind][1] -= ay;
                    // accelerations[p2_ind][2] = -fz; // For 3D
                }
            }
            // Reduce local accelerations to global accelerations
#pragma omp critical
            {
                currentPE += localPE;  // Accumulate potential energy from all threads
                for (int i = 0; i < numParticles; i++) {
                    accelerations[i][0] += localAccelerations[i][0];
                    accelerations[i][1] += localAccelerations[i][1];

                    // accelerations[i][2] += localAccelerations[i][2]; // For 3D
                }
            }
        }
    }

    void verletStep(int t) {
        // Implement the logic to perform a single Verlet integration step
        currentKE_times2 = 0.0;               // Reset kinetic energy before calculation
#pragma omp parallel if (numParticles > 100)  // Only parallelize if there are enough particles to justify the overhead
        {
#pragma omp for schedule(static)
            for (int i = 0; i < numParticles; i++) {
                std::array<double, d>& old_pos = getPosition(t - 1, i);
                std::array<double, d>& pos = getPosition(t, i);
                std::array<double, d>& accel = accelerations[i];
                std::array<double, d>& vel = velocities[i];

                // Update velocities using the new accelerations
                vel[0] += accel[0] * timeStep * 0.5;  // Update velocity by half a time step for the first half of the Verlet algorithm
                vel[1] += accel[1] * timeStep * 0.5;
                // vel[2] += accel[2] * timeStep * 0.5; // For 3D

                // Update positions using the new velocities
                pos[0] = old_pos[0] + vel[0] * timeStep;
                pos[1] = old_pos[1] + vel[1] * timeStep;
                // pos[2] = old_pos[2] + vel[2] * timeStep; // For 3D

                // Apply periodic boundary conditions
                pos[0] -= L * std::floor(pos[0] / L);
                pos[1] -= L * std::floor(pos[1] / L);
                // pos[2] = fmod(pos[2] + L, L); // For 3D
            }
        }

        calculateAccelerations(t);            // Recalculate accelerations based on the new positions
                                              // Update velocities again using the new accelerations
#pragma omp parallel if (numParticles > 100)  // Only parallelize if there are enough particles to justify the overhead
        {
            double localKE_times2 = 0.0;  // Thread-local storage for kinetic energy
#pragma omp for schedule(static)
            for (int i = 0; i < numParticles; i++) {
                std::array<double, d>& vel = velocities[i];
                std::array<double, d>& accel = accelerations[i];
                vel[0] += accel[0] * timeStep * 0.5;
                vel[1] += accel[1] * timeStep * 0.5;
                // vel[2] += accel[2] * timeStep * 0.5; // For 3D

                localKE_times2 += (vel[0] * vel[0] + vel[1] * vel[1]);  // For 3D
            }
#pragma omp critical
            {
                currentKE_times2 += localKE_times2;  // Accumulate kinetic energy from all threads (multiply by 2 to convert from KE to 2*KE)
            }
        }

        energyCalculations(t);  // Perform energy calculations and apply any energy changes based on the energyFunction vector
    }

    void energyCalculations(int t) {
        if (energyFunctionIndex < energyFunction.size() && t == energyFunction[energyFunctionIndex].first && currentKE_times2 > 1) {
            double desiredEnergyChange = energyFunction[energyFunctionIndex].second;  // Get the desired energy change for this time step

            double scaleingFactor =
                std::sqrt(1.0 + 2 * desiredEnergyChange / currentKE_times2);  // Calculate scaling factor based on the desired energy change
#pragma omp parallel for schedule(static) if (numParticles > 200)  // Only parallelize if there are enough particles to justify the overhead
            for (int i = 0; i < numParticles; i++) {
                velocities[i][0] *= scaleingFactor;
                velocities[i][1] *= scaleingFactor;
            }
            currentKE_times2 += 2 * desiredEnergyChange;  // Update kinetic energy to reflect the change
            energyFunctionIndex++;
        }
        potentialEnergies[t] = currentPE;
        kineticEnergies[t] = currentKE_times2 * 0.5;  // Convert from 2*KE to KE
        totalEnergies[t] = currentPE + currentKE_times2 * 0.5;
        temperatures[t] = currentKE_times2 / (numParticles * d);  // Calculate temperature using the kinetic energy and degrees of freedom
    }
    // #pragma omp critical
    //{
    //     currentKE += localKE;  // Accumulate kinetic energy from all threads
    // }
    //} End of parallel region

    void runSimulation() {
        calculateAccelerations(0);  // Calculate initial accelerations based on the initial positions
        energyCalculations(0);      // Perform initial energy calculations
        int five_percent_of_time_steps = static_cast<int>(timeSteps / 20);
        for (int t = 1; t < timeSteps; t++) {
            verletStep(t);  // Perform subsequent Verlet steps
            if (t % five_percent_of_time_steps == 0) {
                std::cout << "Completed " << (t * 100) / timeSteps << "% of simulation." << std::endl;
            }
        }
    }

    void saveResultsToNPZ(const std::string& filename) {
        // Implement the logic to save the results (positions, energies, etc.) to a file
        // Cast to double* so sizeof(T)==8 and shape {timeSteps,numParticles,d} is correct.
        // Passing std::array<double,d>* would double-count d in the byte count and overread.
        cnpy::npz_save(filename, "positions", reinterpret_cast<double*>(positions.data()), {(size_t)timeSteps, (size_t)numParticles, (size_t)d}, "w");
        cnpy::npz_save(filename, "velocities", reinterpret_cast<double*>(velocities.data()), {(size_t)numParticles, (size_t)d}, "a");
        cnpy::npz_save(filename, "accelerations", reinterpret_cast<double*>(accelerations.data()), {(size_t)numParticles, (size_t)d}, "a");
        cnpy::npz_save(filename, "temperatures", temperatures.data(), {timeSteps}, "a");
        cnpy::npz_save(filename, "potentialEnergies", potentialEnergies.data(), {timeSteps}, "a");
        cnpy::npz_save(filename, "kineticEnergies", kineticEnergies.data(), {timeSteps}, "a");
        cnpy::npz_save(filename, "totalEnergies", totalEnergies.data(), {timeSteps}, "a");

        std::vector<double> metadata = {L, static_cast<double>(numParticles), static_cast<double>(timeSteps), finalTime};
        cnpy::npz_save(filename, "metadata", metadata.data(), {metadata.size()}, "a");
    }

    void saveEnergyToCSV(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        // Write header
        file << "TimeStep,Temperature,PotentialEnergy,KineticEnergy,TotalEnergy\n";
        // Write data
        for (int t = 0; t < timeSteps; t++) {
            file << t << "," << temperatures[t] << "," << potentialEnergies[t] << "," << kineticEnergies[t] << "," << totalEnergies[t] << "\n";
        }
        file.close();
    }

    void savePositionsToCSV(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        // Write header
        file << "TimeStep,ParticleIndex,X,Y\n";
        // Write data
        for (int t = 0; t < timeSteps; t++) {
            for (int i = 0; i < numParticles; i++) {
                std::array<double, d>& pos = getPosition(t, i);
                file << t << "," << i << "," << pos[0] << "," << pos[1] << "\n";
            }
        }
        file.close();
    }

    void save() {
        // Implement the logic to save the results (positions, energies, etc.) to a file
        // create a directory within the ./output dir titled
        // "out_{i}" where i is the next available integer (i.e. if there are already 3 directories in output, the next one will be out_4)
        std::string outputDir = "./output/";
        int i = 0;
        while (std::filesystem::exists(outputDir + "out_" + std::to_string(i))) {
            i++;
        }
        outputDir += "out_" + std::to_string(i);
        std::filesystem::create_directories(outputDir);
        saveResultsToNPZ(outputDir + "/results.npz");
        saveEnergyToCSV(outputDir + "/energy_data.csv");
        savePositionsToCSV(outputDir + "/positions_data.csv");
    }
};

std::vector<std::pair<int, double>> buildEnergyFunction(int totalTimeSteps) {
    std::vector<std::pair<int, double>> energyFunction;
    // For the first 5% of time steps, dont do anything
    // then for the next 45% of time steps, add energy to the system at a rate of 1 unit of energy per 1% of time steps
    // then for the next 45% of time steps, remove energy from the system at a rate of 1 unit of energy per 1% of time steps
    // then for the last 5% of time steps, dont do anything
    int fivePercent = totalTimeSteps / 20;
    for (int i = fivePercent; i < totalTimeSteps / 2; i++) {
        energyFunction.push_back({i, 40.0 / (totalTimeSteps / 100)});
    }
    for (int i = totalTimeSteps / 2; i < totalTimeSteps - fivePercent; i++) {
        energyFunction.push_back({i, -40.1 / (totalTimeSteps / 100)});
    }

    return energyFunction;
};

#endif  // PROCESSING_H