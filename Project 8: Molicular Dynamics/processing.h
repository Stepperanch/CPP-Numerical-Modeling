#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#define _USE_MATH_DEFINES

#include <omp.h>

#include "cnpy.h"

#ifndef PROCESSING_H
#define PROCESSING_H

/**
 * This header file contains the core logic for the molecular dynamics simulation, including:
 * - The MolucularSystem class, which encapsulates the state and behavior of the system being simulated
 * - Functions for parsing the configuration file, building the energy function, and generating initial particle positions based on specified shapes
 * and parameters
 * - The Verlet integration algorithm for updating particle positions and velocities over time
 * The code is designed to be flexible and extensible, allowing for different initial configurations, energy change schedules, and boundary conditions
 * to be easily implemented through the configuration file.
 *
 * Author: Nels Buhrley
 * Date: June 2024
 *
 */

// Use doxey style comments for documentation generation

/** @brief Parses a configuration file and returns a map of key-value pairs.
 * The configuration file should have lines in the format "key=value". Lines that are empty or start with '#' will be ignored.
 * @param filename The path to the configuration file.
 * @return A map containing the parsed key-value pairs.
 */
std::map<std::string, std::string> parseConfigFile(const std::string& filename) {
    std::map<std::string, std::string> configData;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << "\n";
        return configData;  // Return an empty map if it fails
    }

    std::string line;
    // Read the file line by line
    while (std::getline(file, line)) {
        // 1. Skip empty lines or lines that start with a comment (#)
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 2. Find where the '=' sign is on this line
        size_t delimiterPos = line.find('=');

        // 3. If we actually found an '=', split the string!
        if (delimiterPos != std::string::npos) {
            // Extract everything before the '='
            std::string key = line.substr(0, delimiterPos);

            // Extract everything after the '='
            std::string value = line.substr(delimiterPos + 1);

            // 4. Save it into our map
            configData[key] = value;
        }
    }

    file.close();
    return configData;
}

/** @brief Splits a string into a vector of strings based on a delimiter.
 * @param input The string to split.
 * @param delimiter The character to split on.
 * @return A vector of strings.
 */
std::vector<std::string> splitToVector(const std::string& input, char delimiter = ',') {
    std::vector<std::string> result;
    std::stringstream ss(input);  // Put our string into the stream
    std::string item;
    // Read from the stream until we hit the delimiter (the comma)
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);  // Add the chunk to our vector
    }

    return result;
}

/** @brief Builds an energy function based on a set of instructions.
 * @param energyInstructions A string containing the energy instructions.
 * @param totalTimeSteps The total number of time steps.
 * @param numParticles The number of particles.
 * @return A vector of doubles representing the energy function.
 */
std::vector<double> buildEnergyFunction(std::string energyInstructions, int totalTimeSteps, int numParticles) {
    std::vector<std::string> instructions_string = splitToVector(energyInstructions, ';');
    std::vector<std::array<double, 3>> instructions;

    for (const std::string& instruction : instructions_string) {
        std::vector<std::string> parts = splitToVector(instruction, ',');
        if (parts.size() != 3) {
            std::cerr << "Invalid energy instruction format: " << instruction << "\n";
            continue;  // Skip invalid instructions
        }
        instructions.push_back({std::stod(parts[0]), std::stod(parts[1]), std::stod(parts[2])});
    }

    // Energy Instructions format: "Start Percent, End Percent, Total Energy Change Per 100 particles; Start Percent, End Percent, Total Energy Change
    // Per 100 particles; ..." Declare a vector to hold the energy change for each time step, initialized to zero with size equal to totalTimeSteps
    std::vector<double> energyFunction(totalTimeSteps, 0.0);  // Initialize with zeros

    int onePercent = totalTimeSteps / 100;
    float nP = static_cast<float>(numParticles);

    for (const auto& instr : instructions) {
        int startStep = static_cast<int>(instr[0] * onePercent);
        int endStep = static_cast<int>(instr[1] * onePercent);
        double energyChangePer100Particles = instr[2];
        double energyChangePerStep = energyChangePer100Particles / 100.0 * nP /
                                     (endStep - startStep);  // Distribute the total energy change evenly across the specified time steps

        for (int i = startStep; i < endStep; i++) {
            energyFunction[i] +=
                energyChangePerStep;  // Add the energy change for this instruction to the energy function for each relevant time step
        }
    }
    return energyFunction;
};


/** @brief Builds a rectangular grid of particles.
 * @param centerX The x-coordinate of the center of the rectangle.
 * @param centerY The y-coordinate of the center of the rectangle.
 * @param rotation The rotation angle of the rectangle.
 * @param spacing The spacing between particles.
 * @param numParticlesX The number of particles along the x-axis.
 * @param numParticlesY The number of particles along the y-axis.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @return A vector of arrays representing the positions of the particles.
 */
std::vector<std::array<double, 2>> buildRectangle(double centerX, double centerY, double rotation, double spacing, int numParticlesX,
                                                  int numParticlesY, float width, float height) {
    std::vector<std::array<double, 2>> positions;
    double cosTheta = std::cos(rotation);
    double sinTheta = std::sin(rotation);

    for (int i = 0; i < numParticlesX; i++) {
        for (int j = 0; j < numParticlesY; j++) {
            double x = (i - numParticlesX / 2.0) * spacing;
            double y = (j - numParticlesY / 2.0) * spacing;

            // Rotate the position about the center of the shape
            double rotatedX = x * cosTheta - y * sinTheta;
            double rotatedY = x * sinTheta + y * cosTheta;

            x = rotatedX + centerX;
            y = rotatedY + centerY;

            if (x < 0 || x > width || y < 0 || y > height) {
                continue;  // Skip particles that are outside the box dimensions
            }

            positions.push_back({x, y});
        }
    }
    return positions;
}

/** @brief Builds a rhombus grid of particles.
 * @param centerX The x-coordinate of the center of the rhombus.
 * @param centerY The y-coordinate of the center of the rhombus.
 * @param rotation The rotation angle of the rhombus.
 * @param spacing The spacing between particles.
 * @param radius The radius of the rhombus.
 * @param boxWidth The width of the bounding box.
 * @param boxHeight The height of the bounding box.
 * @param offsetX The x-coordinate of the offset for the grid.
 * @param offsetY The y-coordinate of the offset for the grid.
 * @param shapeFilter A function to filter which particles to include.
 * @return A vector of arrays representing the positions of the particles.
 */
std::vector<std::array<double, 2>> buildRhombusGrid(double centerX, double centerY, double rotation, double spacing, int radius, double boxWidth,
                                                    double boxHeight, double offsetX, double offsetY,
                                                    std::function<bool(int q, int r, double localX, double localY)> shapeFilter = nullptr) {
    std::vector<std::array<double, 2>> positions;
    double cosTheta = std::cos(rotation);
    double sinTheta = std::sin(rotation);
    double sqrt3 = std::sqrt(3.0);

    for (int q = -radius; q <= radius; q++) {
        for (int r = -radius; r <= radius; r++) {
            // Convert axial to Cartesian, AND apply our custom center offset
            double localX = (spacing * (q + 0.5 * r)) - offsetX;
            double localY = (spacing * ((sqrt3 / 2.0) * r)) - offsetY;

            // The Filter Cutout
            if (shapeFilter != nullptr && !shapeFilter(q, r, localX, localY)) {
                continue;
            }

            // Rotate the coordinates
            double rotatedX = localX * cosTheta - localY * sinTheta;
            double rotatedY = localX * sinTheta + localY * cosTheta;

            // Move to final position on screen
            double finalX = rotatedX + centerX;
            double finalY = rotatedY + centerY;

            // Screen boundary check
            if (finalX < 0 || finalX > boxWidth || finalY < 0 || finalY > boxHeight) {
                continue;
            }

            positions.push_back({finalX, finalY});
        }
    }
    return positions;
}

/** @brief Builds a rhombus of particles.
 * @param cX The x-coordinate of the center of the rhombus.
 * @param cY The y-coordinate of the center of the rhombus.
 * @param rot The rotation angle of the rhombus.
 * @param space The spacing between particles.
 * @param size The size of the rhombus.
 * @param w The width of the bounding box.
 * @param h The height of the bounding box.
 * @return A vector of arrays representing the positions of the particles.
 */
std::vector<std::array<double, 2>> buildRhombus(double cX, double cY, double rot, double space, int size, double w, double h) {
    // No offset, no filter. Just return the raw grid!
    return buildRhombusGrid(cX, cY, rot, space, size, w, h, 0.0, 0.0, [](int q, int r, double localX, double localY) {
        return true;  // Keep everything
    });
}

/** @brief Builds a hexagonal grid of particles.
 * @param cX The x-coordinate of the center of the hexagon.
 * @param cY The y-coordinate of the center of the hexagon.
 * @param rot The rotation angle of the hexagon.
 * @param space The spacing between particles.
 * @param size The size of the hexagon.
 * @param w The width of the bounding box.
 * @param h The height of the bounding box.
 * @return A vector of arrays representing the positions of the particles.
 */
std::vector<std::array<double, 2>> buildHexagon(double cX, double cY, double rot, double space, int size, double w, double h) {
    return buildRhombusGrid(cX, cY, rot, space, size, w, h, 0.0, 0.0, [size](int q, int r, double localX, double localY) {
        return std::abs(q + r) <= size;  // Chop off the rhombus corners
    });
}

/** @brief Builds a triangular grid of particles.
 * @param cX The x-coordinate of the center of the triangle.
 * @param cY The y-coordinate of the center of the triangle.
 * @param rot The rotation angle of the triangle.
 * @param space The spacing between particles.
 * @param size The size of the triangle.
 * @param w The width of the bounding box.
 * @param h The height of the bounding box.
 * @return A vector of arrays representing the positions of the particles.
 */
std::vector<std::array<double, 2>> buildTriangle(double cX, double cY, double rot, double space, int size, double w, double h) {
    // Calculate the true center of mass for this specific triangle
    double offsetX = space * size * 0.5;
    double offsetY = space * size * (std::sqrt(3.0) / 6.0);

    return buildRhombusGrid(cX, cY, rot, space, size, w, h, offsetX, offsetY, [size](int q, int r, double localX, double localY) {
        return (q >= 0) && (r >= 0) && (q + r <= size);  // Restrict to positive axes
    });
}

/** @brief Builds a circular grid of particles.
 * @param cX The x-coordinate of the center of the circle.
 * @param cY The y-coordinate of the center of the circle.
 * @param rot The rotation angle of the circle.
 * @param space The spacing between particles.
 * @param size The size of the circle.
 * @param w The width of the bounding box.
 * @param h The height of the bounding box.
 * @return A vector of arrays representing the positions of the particles.
 */
std::vector<std::array<double, 2>> buildCircle(double cX, double cY, double rot, double space, int size, double w, double h) {
    // Using physical distance instead of grid coordinates
    double maxDistSquared = (size * space) * (size * space);

    return buildRhombusGrid(cX, cY, rot, space, size, w, h, 0.0, 0.0, [maxDistSquared](int q, int r, double localX, double localY) {
        return (localX * localX + localY * localY) <= maxDistSquared;
    });
}

/** @brief Builds initial positions for the molecular system based on instructions.
 * @param initialPositionsInstructions The string containing the initial position instructions.
 * @param width The width of the simulation box.
 * @param height The height of the simulation box.
 * @return A vector of arrays representing the initial positions of the particles.
 */
std::vector<std::array<double, 2>> buildInnitialPositions(std::string initialPositionsInstructions, float width, float height) {
    std::vector<std::string> instructions_string = splitToVector(initialPositionsInstructions, ';');
    std::vector<std::array<double, 2>> allPositions;

    /*
    The initial position instructions format is a string with the following format:
    "Shape (ex. Rectangle, Hexagon, Triangle, Circle),
    Center X,
    Center Y,
    Rotation in radians,
    spacing between particles,
    size parameter 1 (ex. side length of side for square, radius for circle all in units of particles)
    size parameter 2 (ex. side length of other side for rectangle, ignored for other shapes all in units of particles)
    */

    for (const std::string& instruction : instructions_string) {
        std::vector<std::string> parts = splitToVector(instruction, ',');
        std::vector<std::array<double, 2>> shapePositions;

        // For rectangles, we expect 7 parts: Shape, Center X, Center Y, Rotation, Spacing, Size Param 1, Size Param 2
        // For other shapes, we expect 6 parts: Shape, Center X, Center Y, Rotation, Spacing, Size Param 1
        if (parts.size() == 7 && parts[0] == "Rectangle") {
            double cX = std::stod(parts[1]);
            double cY = std::stod(parts[2]);
            double rot = std::stod(parts[3]);
            double space = std::stod(parts[4]);
            int size1 = std::stoi(parts[5]);
            int size2 = std::stoi(parts[6]);
            shapePositions = buildRectangle(cX, cY, rot, space, size1, size2, width, height);
        } else if (parts.size() == 6) {
            double cX = std::stod(parts[1]);
            double cY = std::stod(parts[2]);
            double rot = std::stod(parts[3]);
            double space = std::stod(parts[4]);
            int size = std::stoi(parts[5]);
            std::string shape = parts[0];

            if (shape == "Hexagon") {
                shapePositions = buildHexagon(cX, cY, rot, space, size, width, height);
            } else if (shape == "Triangle") {
                shapePositions = buildTriangle(cX, cY, rot, space, size, width, height);
            } else if (shape == "Circle") {
                shapePositions = buildCircle(cX, cY, rot, space, size, width, height);
            } else if (shape == "Rhombus") {
                shapePositions = buildRhombus(cX, cY, rot, space, size, width, height);
            } else {
                std::cerr << "Unknown shape: " << shape << "\n";
                continue;
            }
        } else {
            std::cerr << "Invalid initial position instruction format: " << instruction << "\n";
            continue;
        }

        // Add all positions from this shape to the total
        allPositions.insert(allPositions.end(), shapePositions.begin(), shapePositions.end());
    }

    return allPositions;
}

class MolucularSystem {
   public:
    static constexpr int d = 2;
    static constexpr float rc = 2.5;       // Cutoff distance for Lennard-Jones potential in units of sigma
    static constexpr float rc2 = rc * rc;  // Square of the cutoff distance for efficiency

    float width;   // Width of the simulation box in the x direction
    float height;  // Height of the simulation box in the y direction

    std::string xBCType;  // Boundary condition in the x direction ("periodic" or "reflective")
    std::string yBCType;  // Boundary condition in the y direction ("periodic" or "reflective")

    // Function pointer types for boundary conditions — set once at construction, called in the hot loop with zero branching on BC type
    using PosBCFn  = void (*)(double& pos, double& vel, double dim);
    using MinImFn  = void (*)(double& delta,              double dim);

    PosBCFn xPosBCFn;   // position+velocity BC for x
    PosBCFn yPosBCFn;   // position+velocity BC for y
    MinImFn xMinImFn;   // minimum-image displacement correction for x
    MinImFn yMinImFn;   // minimum-image displacement correction for y

    float gravity;  // Strength of the constant downward force to simulate gravity

    std::vector<std::array<double, d>> positions;
    std::vector<std::array<double, d>> velocities;
    std::vector<std::array<double, d>> accelerations;

    std::vector<std::array<double, 2>> initialPositions;  // Initial particle positions

    double currentPE;         // Current potential energy of the system
    double currentKE_times2;  // 2 x Current kinetic energy of the system
    double currentTE;         // Current total energy of the system

    std::vector<double> temperatures;
    std::vector<double> potentialEnergies;
    std::vector<double> kineticEnergies;
    std::vector<double> totalEnergies;

    std::vector<double> energyFunction;    // Stores the timesteps and the ∆E values for the system
    unsigned int energyFunctionIndex = 0;  // Index to track the current position in the energyFunction vector

    unsigned int numParticles;
    unsigned int timeSteps;
    double finalTime;
    double timeStep;

    // Assume sigma, epsilon, and mass are all 1 for simplicity in reduced units

    MolucularSystem(std::map<std::string, std::string> config) {
        this->timeSteps = std::stoul(config["timeSteps"]);
        this->finalTime = std::stof(config["finalTime"]);

        this->width = std::stof(config["width"]);
        this->height = std::stof(config["height"]);

        this->xBCType = config["xBoundaryCondition"];
        this->yBCType = config["yBoundaryCondition"];

        this->gravity = std::stof(config["gravity"]);

        this->initialPositions = buildInnitialPositions(config["initialPositionsInstructions"], width, height);

        this->numParticles = initialPositions.size();

        this->energyFunction = buildEnergyFunction(config["EnergyInstructions"], timeSteps, numParticles);

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

        // Assign function pointers once — the if/else runs only at construction, never in the hot loop
        xPosBCFn = (xBCType == "periodic") ? &periodicPositionBC : &reflectivePositionBC;
        yPosBCFn = (yBCType == "periodic") ? &periodicPositionBC : &reflectivePositionBC;
        xMinImFn = (xBCType == "periodic") ? &periodicMinImage   : &noOpMinImage;
        yMinImFn = (yBCType == "periodic") ? &periodicMinImage   : &noOpMinImage;
    }

    // =========================================================================
    // Static BC math implementations — branchless, no if statements
    // =========================================================================

    /** @brief Periodic position BC: wraps pos into [0, dim) using modular arithmetic.
     * Branchless: pos = pos - dim * floor(pos / dim)
     * @param pos The position coordinate to apply the BC to (modified in place).
     * @param vel The velocity coordinate to apply the BC to (not modified for periodic BC
     * @param dim The dimension of the box in this direction (width or height).
     */
    static void periodicPositionBC(double& pos, double& vel, double dim) {
        pos = pos - dim * std::floor(pos / dim);
    }

    /** @brief Reflective position BC: folds pos into [0, dim] using a triangle wave.
     * Uses floor and fmod — no if statements.
     *
     * Derivation:
     *   n      = floor(pos / dim)            → how many half-periods traveled
     *   p      = pos - dim * n               → remainder in [0, dim)
     *   parity = fmod(|n|, 2)               → 0.0 (even bounces) or 1.0 (odd bounces)
     *   pos    = p + parity * (dim - 2*p)   → p if even, (dim-p) if odd  (branchless select)
     *   vel   *= 1 - 2*parity               → unchanged (+1) or flipped (-1)
     *
     * @param pos The position coordinate to apply the BC to (modified in place).
     * @param vel The velocity coordinate to apply the BC to (modified in place to flip on odd bounces).
     * @param dim The dimension of the box in this direction (width or height).
     */
    static void reflectivePositionBC(double& pos, double& vel, double dim) {
        double n      = std::floor(pos / dim);
        double p      = pos - dim * n;
        double parity = std::fmod(std::abs(n), 2.0);  // 0.0 or 1.0
        pos = p + parity * (dim - 2.0 * p);
        vel *= 1.0 - 2.0 * parity;
    }

    /** @brief Periodic minimum-image: folds displacement into [-dim/2, dim/2) using round.
     * dx = dx - dim * round(dx / dim)
     * @param dx The displacement coordinate to apply the minimum-image correction to (modified in place).
     * @param dim The dimension of the box in this direction (width or height).
     */
    static void periodicMinImage(double& dx, double dim) {
        dx -= dim * std::round(dx / dim);
    }

    /** @brief Reflective minimum-image: no-op. Particles cannot cross reflective walls,
     * so no image correction is needed for force calculations.
     * @param dx The displacement coordinate to apply the minimum-image correction to (modified in place).
     * @param dim The dimension of the box in this direction (width or height).
     */
    static void noOpMinImage(double& /*dx*/, double /*dim*/) {}

    // =========================================================================
    // Inline dispatch wrappers — one indirect call per axis, no BC-type branching
    // =========================================================================

    /** @brief Apply periodic or reflective boundary condition to a particle's x position and velocity.
     * For periodic BC, wraps the position back into [0, width).
     * For reflective BC, bounces the particle off the walls and reverses the velocity component.
     * @param pos The x position of the particle (modified in place).
     * @param vel The x velocity of the particle (modified in place).
     */
    inline void applyPositionBC_x(double& pos, double& vel) { xPosBCFn(pos, vel, width);  }

    /** @brief Apply position+velocity BC to y. Dispatches to periodic or reflective via function pointer. */
    inline void applyPositionBC_y(double& pos, double& vel) { yPosBCFn(pos, vel, height); }

    /** @brief Apply minimum-image displacement correction to dx for force calculation. */
    inline void applyMinImageBC_x(double& dx)               { xMinImFn(dx, width);        }

    /** @brief Apply minimum-image displacement correction to dy for force calculation. */
    inline void applyMinImageBC_y(double& dy)               { yMinImFn(dy, height);       }

    /** @brief Get the position of a particle at a specific time step.
     * @param timeStep The time step for which to retrieve the position.
     * @param particleIndex The index of the particle for which to retrieve the position.
     * @return A reference to the position array of the specified particle at the specified time step.
     */
    inline std::array<double, d>& getPosition(unsigned int timeStep, unsigned int particleIndex) {
        return positions[timeStep * numParticles + particleIndex];
    }

    /** @brief Set the position of a particle at a specific time step.
     * @param timeStep The time step for which to set the position.
     * @param particleIndex The index of the particle for which to set the position.
     * @param newPosition The new position to set.
     */
    inline void setPosition(unsigned int timeStep, unsigned int particleIndex, const std::array<double, d>& newPosition) {
        positions[timeStep * numParticles + particleIndex] = newPosition;
    }

    /** @brief Calculate the accelerations for all particles at a specific time step.
     * @param t The time step for which to calculate accelerations.
     */
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
                localPE += gravity * p1_pos[1];            // Add potential energy contribution from the constant downward force to simulate gravity
                localAccelerations[p1_ind][1] -= gravity;  // Add a small constant downward force to simulate gravity

                for (int p2_ind = p1_ind + 1; p2_ind < numParticles; p2_ind++) {
                    std::array<double, d>& p2_pos = getPosition(t, p2_ind);

                    double dx = p1_pos[0] - p2_pos[0];
                    double dy = p1_pos[1] - p2_pos[1];
                    // double dz = p1_pos[2] - p2_pos[2]; // For 3D

                    // Apply minimum image convention based on boundary condition type
                    applyMinImageBC_x(dx);
                    applyMinImageBC_y(dy);

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
                    localAccelerations[p1_ind][1] +=
                        ay;  // Update acceleration for particle 1 and add a small constant downward force to simulate gravity
                    // accelerations[p1_ind][2] += fz; // For 3D

                    localAccelerations[p2_ind][0] -= ax;  // Update acceleration for particle 2 (Newton's third law)
                    localAccelerations[p2_ind][1] -=
                        ay;  // Update acceleration for particle 2 and add a small constant downward force to simulate gravity
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

    /** @brief Perform a single Verlet integration step.
     * @param t The time step for which to perform the integration.
     */
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

                // Apply boundary conditions based on xBCType and yBCType
                applyPositionBC_x(pos[0], vel[0]);
                applyPositionBC_y(pos[1], vel[1]);
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

    /** @brief Perform energy calculations and apply any energy changes based on the energyFunction vector.
     * @param t The time step for which to perform energy calculations.
     */
    void energyCalculations(int t) {
        if (t < (int)energyFunction.size() && energyFunction[t] != 0.0) {
            double desiredEnergyChange = energyFunction[t];  // Energy change scheduled for this timestep

            double scaleingFactor =
                std::sqrt(1.0 + 2 * desiredEnergyChange / currentKE_times2);  // Calculate scaling factor based on the desired energy change
#pragma omp parallel for schedule(static) if (numParticles > 200)  // Only parallelize if there are enough particles to justify the overhead
            for (int i = 0; i < numParticles; i++) {
                if (std::abs(velocities[i][0]) > 1.0 ||
                    scaleingFactor > 1)  // Only scale if the velocity is above a certain threshold or if we are adding energy to the system
                    velocities[i][0] *= scaleingFactor;
                if (std::abs(velocities[i][1]) > 1.0 ||
                    scaleingFactor > 1)  // Only scale if the velocity is above a certain threshold or if we are adding energy to the system
                    velocities[i][1] *= scaleingFactor;
            }
            currentKE_times2 += 2 * desiredEnergyChange;  // Update kinetic energy to reflect the change
        }
        potentialEnergies[t] = currentPE;
        kineticEnergies[t] = currentKE_times2 * 0.5;  // Convert from 2*KE to KE
        totalEnergies[t] = currentPE + currentKE_times2 * 0.5;
        temperatures[t] = currentKE_times2 / (numParticles * d);  // Calculate temperature using the kinetic energy and degrees of freedom
    }

    /** @brief Run the simulation for the specified number of time steps.
     */
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

    /** @brief Save the simulation results to .npy files.
     * @param directory The directory where the files will be saved.
     */
    void saveResultsToNpy(const std::string& directory) {
        // Save each array as a separate .npy file to avoid the 4 GB ZIP/NPZ size limit.
        // Only save one in 5 positions to reduce file size; all data saved as float.
        std::vector<float> positions_float, temperatures_float, potentialEnergies_float, kineticEnergies_float, totalEnergies_float;
        positions_float.resize((timeSteps / 5) * numParticles * d);
        temperatures_float.resize(timeSteps);
        potentialEnergies_float.resize(timeSteps);
        kineticEnergies_float.resize(timeSteps);
        totalEnergies_float.resize(timeSteps);

        int skip = 10;

#pragma omp parallel for collapse(3) schedule(static) if ((timeSteps / 5) * numParticles * d > 1000)
        for (int t = 0; t < timeSteps; t += skip) {
            for (int i = 0; i < numParticles; i++) {
                for (int j = 0; j < d; j++) {
                    int idx = ((t / skip) * numParticles + i) * d + j;
                    positions_float[idx] = static_cast<float>(positions[(t * numParticles + i)][j]);
                }
            }
        }

// Convert scalar time-series (these are small, no need for skip)
#pragma omp parallel for schedule(static) if (timeSteps > 1000)
        for (int t = 0; t < (int)timeSteps; t++) {
            temperatures_float[t] = static_cast<float>(temperatures[t]);
            potentialEnergies_float[t] = static_cast<float>(potentialEnergies[t]);
            kineticEnergies_float[t] = static_cast<float>(kineticEnergies[t]);
            totalEnergies_float[t] = static_cast<float>(totalEnergies[t]);
        }

        cnpy::npy_save(directory + "/positions.npy", positions_float.data(), {(size_t)(timeSteps / 5), (size_t)numParticles, (size_t)d}, "w");
        cnpy::npy_save(directory + "/temperatures.npy", temperatures_float.data(), {timeSteps}, "w");
        cnpy::npy_save(directory + "/potentialEnergies.npy", potentialEnergies_float.data(), {timeSteps}, "w");
        cnpy::npy_save(directory + "/kineticEnergies.npy", kineticEnergies_float.data(), {timeSteps}, "w");
        cnpy::npy_save(directory + "/totalEnergies.npy", totalEnergies_float.data(), {timeSteps}, "w");

        std::vector<double> metadata = {static_cast<double>(width), static_cast<double>(height), static_cast<double>(numParticles),
                                        static_cast<double>(timeSteps), finalTime};
        cnpy::npy_save(directory + "/metadata.npy", metadata.data(), {metadata.size()}, "w");
    }

    /** @brief Save energy data to a CSV file.
     * @param filename The name of the CSV file to save.
     */
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

    /** @brief Save position data to a CSV file.
     * @param filename The name of the CSV file to save.
     */
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

    /** @brief Save the simulation results to files.
     */
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
        saveResultsToNpy(outputDir);
        saveEnergyToCSV(outputDir + "/energy_data_" + std::to_string(i) + ".csv");
        savePositionsToCSV(outputDir + "/positions_data_" + std::to_string(i) + ".csv");
    }
};

#endif  // PROCESSING_H