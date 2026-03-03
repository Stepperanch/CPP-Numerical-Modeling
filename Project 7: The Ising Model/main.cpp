#include <iostream>
#include <vector>
#include <random>
#include <omp.h>
#include "processing.h"

int main() {
    int N = 100;
    int iterations = 200;

    // 1. Define your temperatures
    float minTemp = 0.01;
    float maxTemp = 6.0;
    int tempSteps = 10;

    // 2. Define your magnetic field range
    float hmax = 10;
    float hmin = -hmax;
    int numHSteps = 10;

    // 2. Run the simulation across the temperature range
    // parameters: lattice size, iterations, hMin, hMax, numHSteps, tempMin, tempMax, numTempSteps
    Simulation simulation(N, iterations, hmin, hmax, numHSteps, minTemp, maxTemp, tempSteps);
    simulation.runIsingSimulation();
    simulation.saveResults();

    return 0;
}