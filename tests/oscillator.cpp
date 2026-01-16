#include <boost/numeric/odeint.hpp>
#include <iostream>
#include <vector>

using namespace boost::numeric; //odeint

// State: [position, velocity]
typedef std::vector<double> state_type;

// The System: A damped harmonic oscillator
// d2x/dt2 + gamma*dx/dt + w0^2*x = 0
void damped_oscillator(const state_type& x, state_type& dxdt, const double t) {
    const double gamma = 0.15;  // friction/damping
    const double w0 = 1.0;      // natural frequency

    dxdt[0] = x[1];                            // v = dx/dt
    dxdt[1] = -gamma * x[1] - w0 * w0 * x[0];  // a = -gamma*v - w0^2*x
}

// Observer to print results to the terminal
void write_results(const state_type& x, const double t) {
    std::cout << t << ", " << x[0] << ", " << x[1] << std::endl;
}

int main() {
    state_type x = {1.0, 0.0};  // Initial position 1, velocity 0

    // Setup the 8th-order controlled stepper for extreme precision
    // absolute error: 1e-12, relative error: 1e-12
    auto stepper = odeint::make_controlled(
        1.0e-12, 1.0e-12, odeint::runge_kutta_fehlberg78<state_type>());

    std::cout << "time, position, velocity" << std::endl;

    // Integrate from t=0 to t=20
    odeint::integrate_adaptive(stepper, damped_oscillator, x, 0.0, 20.0, 0.01,
                                               write_results);

    return 0;
}