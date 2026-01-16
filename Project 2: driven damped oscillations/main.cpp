#include <iostream>
#include <fstream>
#include <vector>


#include "oscillator.h"
#include "processing.h"

int main() {
    std::cout << "Driven Damped Oscillator Simulation" << std::endl;

    testOscillator osc;

    osc.printParameters();

    auto derivFunc = [&osc](const std::vector<double>& state, std::vector<double>& derivatives,
                            double time) { osc.computeDerivatives(state, derivatives, time); };

    // Stop condition: continue while time < 180.0 seconds
    auto stopCondition = [](const std::vector<double>& state) {
        return state[0] < 180.0;  // state[0] is time
    };

    double timeStep = 0.04;  // Time step for the simulation

    std::vector<double> initialState = osc.getState();
    std::vector<std::vector<double>> path =
        rk4Simulation(initialState, derivFunc, stopCondition, timeStep);


    std::cout << "Simulation complete. Total steps: " << path.size() << std::endl;
    std::cout << "Initial state: Time = " << path.front()[0]
              << ", Angle = " << path.front()[1]
              << ", Angular Velocity = " << path.front()[2] << std::endl;
    std::cout << "Final state: Time = " << path.back()[0]
              << ", Angle = " << path.back()[1]
              << ", Angular Velocity = " << path.back()[2] << std::endl;

    //outpur results to a file in the directory ./Output/oscillator_output.csv
    std::ofstream outFile("/Users/nelsbuhrley/CPP_Workspace/Project 2: driven damped oscillations/Output/oscillator_output.csv");
    if (outFile.is_open()) {
        outFile << "Time,Angle,AngularVelocity\n";
        for (const auto& state : path) {
            outFile << state[0] << "," << state[1] << "," << state[2] << "\n";
        }
        outFile.close();
        std::cout << "Results written to Output/oscillator_output.csv" << std::endl;
    } else {
        std::cerr << "Error: Unable to open output file." << std::endl;
    }



    return 0;
}
