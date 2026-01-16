#pragma once

#include <functional>
#include <vector>

// TODO: Declare your data processing functions here.

typedef std::vector<double> state_type;

// RK4 simulation function
std::vector<state_type> rk4Simulation(
    state_type& state, std::function<void(const state_type&, state_type&, double)> derivatives,
    std::function<bool(const state_type&)> stopCondition, double timeStep);