#include "processing.h"

#include <functional>
#include <vector>
#include <iostream>

typedef std::vector<double> state_type;

std::vector<state_type> rk4Simulation(
    state_type& state, std::function<void(const state_type&, state_type&, double)> derivatives,
    std::function<bool(const state_type&)> stopCondition, double timeStep) {
    std::vector<state_type> trajectory;
    trajectory.push_back(state);

    state_type k1(state.size());
    state_type k2(state.size());
    state_type k3(state.size());
    state_type k4(state.size());
    state_type tempState(state.size());

    while (stopCondition(state)) {
        // k1
        derivatives(state, k1, 0.0);

        // k2
        for (size_t i = 0; i < state.size(); ++i) {
            tempState[i] = state[i] + 0.5 * timeStep * k1[i];
        }
        derivatives(tempState, k2, 0.5 * timeStep);

        // k3
        for (size_t i = 0; i < state.size(); ++i) {
            tempState[i] = state[i] + 0.5 * timeStep * k2[i];
        }
        derivatives(tempState, k3, 0.5 * timeStep);

        // k4
        for (size_t i = 0; i < state.size(); ++i) {
            tempState[i] = state[i] + timeStep * k3[i];
        }
        derivatives(tempState, k4, timeStep);

        // Update state
        for (size_t i = 0; i < state.size(); ++i) {
            state[i] += (timeStep / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
        }

        trajectory.push_back(state);
        //std::cout << "Current time: " << state[0] << " seconds\r" << std::endl;
    }
    return trajectory;
}