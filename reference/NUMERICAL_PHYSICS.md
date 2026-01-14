# Numerical Physics with C++

## 📚 Table of Contents
1. [Numerical Methods Basics](#numerical-methods-basics)
2. [Differential Equations](#differential-equations)
3. [Physics Simulations](#physics-simulations)
4. [Data Analysis](#data-analysis)
5. [Optimization Techniques](#optimization-techniques)
6. [Monte Carlo Methods](#monte-carlo-methods)

---

## Numerical Methods Basics

### Floating-Point Precision
```cpp
#include <iostream>
#include <iomanip>
#include <cmath>

int main() {
    // Use double for physics calculations
    double value = 1.0 / 3.0;
    
    // Set output precision
    std::cout << std::setprecision(15) << std::fixed;
    std::cout << "1/3 = " << value << std::endl;
    
    // Machine epsilon (smallest difference)
    double epsilon = std::numeric_limits<double>::epsilon();
    std::cout << "Machine epsilon: " << epsilon << std::endl;
    
    // Comparison with tolerance
    double a = 0.1 + 0.1 + 0.1;
    double b = 0.3;
    
    if (std::abs(a - b) < 1e-10) {
        std::cout << "Values are approximately equal" << std::endl;
    }
    
    return 0;
}
```

### Numerical Derivatives
```cpp
#include <cmath>
#include <functional>

// Forward difference
double derivative(std::function<double(double)> f, double x, double h = 1e-5) {
    return (f(x + h) - f(x)) / h;
}

// Central difference (more accurate)
double derivativeCentral(std::function<double(double)> f, double x, double h = 1e-5) {
    return (f(x + h) - f(x - h)) / (2 * h);
}

// Example usage
int main() {
    // f(x) = x^2
    auto f = [](double x) { return x * x; };
    
    double x = 2.0;
    double dfdx = derivativeCentral(f, x);
    
    std::cout << "f(x) = x^2 at x = 2" << std::endl;
    std::cout << "Numerical derivative: " << dfdx << std::endl;
    std::cout << "Analytical derivative (2x): " << 2 * x << std::endl;
    
    return 0;
}
```

### Numerical Integration
```cpp
#include <cmath>
#include <functional>

// Trapezoidal rule
double integrate(std::function<double(double)> f, double a, double b, int n = 1000) {
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    
    for (int i = 1; i < n; ++i) {
        sum += f(a + i * h);
    }
    
    return sum * h;
}

// Simpson's rule (more accurate)
double integrateSimpson(std::function<double(double)> f, double a, double b, int n = 1000) {
    if (n % 2 == 1) n++;  // Ensure n is even
    
    double h = (b - a) / n;
    double sum = f(a) + f(b);
    
    for (int i = 1; i < n; i += 2) {
        sum += 4 * f(a + i * h);
    }
    
    for (int i = 2; i < n - 1; i += 2) {
        sum += 2 * f(a + i * h);
    }
    
    return sum * h / 3;
}

// Example: Integrate sin(x) from 0 to π
int main() {
    auto f = [](double x) { return std::sin(x); };
    
    double result = integrateSimpson(f, 0, M_PI);
    std::cout << "Integral of sin(x) from 0 to π: " << result << std::endl;
    std::cout << "Analytical result: 2.0" << std::endl;
    
    return 0;
}
```

### Root Finding (Newton-Raphson)
```cpp
#include <cmath>
#include <functional>

double findRoot(std::function<double(double)> f,
                std::function<double(double)> df,
                double x0, double tol = 1e-10, int maxIter = 100) {
    double x = x0;
    
    for (int i = 0; i < maxIter; ++i) {
        double fx = f(x);
        double dfx = df(x);
        
        if (std::abs(fx) < tol) {
            return x;
        }
        
        x = x - fx / dfx;
    }
    
    return x;
}

// Example: Find root of x^2 - 2 (sqrt(2))
int main() {
    auto f = [](double x) { return x * x - 2; };
    auto df = [](double x) { return 2 * x; };
    
    double root = findRoot(f, df, 1.0);
    std::cout << "Root of x^2 - 2: " << root << std::endl;
    std::cout << "sqrt(2): " << std::sqrt(2) << std::endl;
    
    return 0;
}
```

---

## Differential Equations

### Euler Method (First-Order ODE)
```cpp
#include <vector>
#include <functional>

struct State {
    double t;
    double y;
};

std::vector<State> eulerMethod(
    std::function<double(double, double)> dydt,
    double y0, double t0, double tf, double dt) {
    
    std::vector<State> solution;
    double t = t0;
    double y = y0;
    
    while (t <= tf) {
        solution.push_back({t, y});
        y += dydt(t, y) * dt;
        t += dt;
    }
    
    return solution;
}

// Example: dy/dt = -y (exponential decay)
int main() {
    auto dydt = [](double t, double y) { return -y; };
    
    auto solution = eulerMethod(dydt, 1.0, 0.0, 5.0, 0.01);
    
    std::cout << "t\ty\tExact" << std::endl;
    for (size_t i = 0; i < solution.size(); i += 100) {
        double t = solution[i].t;
        double y = solution[i].y;
        double exact = std::exp(-t);
        std::cout << t << "\t" << y << "\t" << exact << std::endl;
    }
    
    return 0;
}
```

### Runge-Kutta 4th Order (RK4)
```cpp
#include <vector>
#include <functional>

struct State {
    double t;
    double y;
};

std::vector<State> rk4Method(
    std::function<double(double, double)> dydt,
    double y0, double t0, double tf, double dt) {
    
    std::vector<State> solution;
    double t = t0;
    double y = y0;
    
    while (t <= tf) {
        solution.push_back({t, y});
        
        // RK4 coefficients
        double k1 = dydt(t, y);
        double k2 = dydt(t + dt/2, y + dt*k1/2);
        double k3 = dydt(t + dt/2, y + dt*k2/2);
        double k4 = dydt(t + dt, y + dt*k3);
        
        y += (dt / 6) * (k1 + 2*k2 + 2*k3 + k4);
        t += dt;
    }
    
    return solution;
}

// Example: Simple harmonic oscillator
// d²x/dt² = -ω²x
// Convert to two first-order ODEs:
// dx/dt = v
// dv/dt = -ω²x

struct State2D {
    double t;
    double x;
    double v;
};

std::vector<State2D> harmonicOscillator(double x0, double v0, double omega,
                                         double t0, double tf, double dt) {
    std::vector<State2D> solution;
    double t = t0;
    double x = x0;
    double v = v0;
    
    while (t <= tf) {
        solution.push_back({t, x, v});
        
        // RK4 for coupled ODEs
        double k1x = v;
        double k1v = -omega * omega * x;
        
        double k2x = v + dt * k1v / 2;
        double k2v = -omega * omega * (x + dt * k1x / 2);
        
        double k3x = v + dt * k2v / 2;
        double k3v = -omega * omega * (x + dt * k2x / 2);
        
        double k4x = v + dt * k3v;
        double k4v = -omega * omega * (x + dt * k3x);
        
        x += (dt / 6) * (k1x + 2*k2x + 2*k3x + k4x);
        v += (dt / 6) * (k1v + 2*k2v + 2*k3v + k4v);
        t += dt;
    }
    
    return solution;
}

int main() {
    // Initial conditions: x(0) = 1, v(0) = 0
    // Angular frequency: ω = 1
    auto solution = harmonicOscillator(1.0, 0.0, 1.0, 0.0, 10.0, 0.01);
    
    std::cout << "t\tx\tv" << std::endl;
    for (size_t i = 0; i < solution.size(); i += 100) {
        std::cout << solution[i].t << "\t" 
                  << solution[i].x << "\t" 
                  << solution[i].v << std::endl;
    }
    
    return 0;
}
```

---

## Physics Simulations

### Projectile Motion
```cpp
#include <cmath>
#include <vector>

struct Vector2D {
    double x, y;
    
    Vector2D operator+(const Vector2D& other) const {
        return {x + other.x, y + other.y};
    }
    
    Vector2D operator*(double scalar) const {
        return {x * scalar, y * scalar};
    }
};

struct Projectile {
    Vector2D position;
    Vector2D velocity;
};

class ProjectileSimulation {
private:
    const double g = 9.81;  // m/s²
    double dt;
    
public:
    ProjectileSimulation(double timestep = 0.01) : dt(timestep) {}
    
    std::vector<Projectile> simulate(double v0, double angle, double maxTime) {
        std::vector<Projectile> trajectory;
        
        // Initial conditions
        Projectile proj;
        proj.position = {0, 0};
        proj.velocity = {v0 * std::cos(angle), v0 * std::sin(angle)};
        
        double t = 0;
        while (t < maxTime && proj.position.y >= 0) {
            trajectory.push_back(proj);
            
            // Update velocity (gravity)
            proj.velocity.y -= g * dt;
            
            // Update position
            proj.position.x += proj.velocity.x * dt;
            proj.position.y += proj.velocity.y * dt;
            
            t += dt;
        }
        
        return trajectory;
    }
    
    double getRange(double v0, double angle) {
        return (v0 * v0 * std::sin(2 * angle)) / g;
    }
    
    double getMaxHeight(double v0, double angle) {
        return (v0 * v0 * std::sin(angle) * std::sin(angle)) / (2 * g);
    }
};

int main() {
    ProjectileSimulation sim(0.01);
    
    double v0 = 50.0;  // m/s
    double angle = M_PI / 4;  // 45 degrees
    
    auto trajectory = sim.simulate(v0, angle, 10.0);
    
    std::cout << "Initial velocity: " << v0 << " m/s" << std::endl;
    std::cout << "Launch angle: " << (angle * 180 / M_PI) << " degrees" << std::endl;
    std::cout << "Theoretical range: " << sim.getRange(v0, angle) << " m" << std::endl;
    std::cout << "Theoretical max height: " << sim.getMaxHeight(v0, angle) << " m" << std::endl;
    
    // Print some trajectory points
    std::cout << "\nTrajectory:" << std::endl;
    std::cout << "t\tx\ty" << std::endl;
    for (size_t i = 0; i < trajectory.size(); i += 50) {
        double t = i * 0.01;
        std::cout << t << "\t" 
                  << trajectory[i].position.x << "\t" 
                  << trajectory[i].position.y << std::endl;
    }
    
    return 0;
}
```

### N-Body Problem (Gravitational Simulation)
```cpp
#include <vector>
#include <cmath>

struct Vector3D {
    double x, y, z;
    
    Vector3D operator+(const Vector3D& v) const {
        return {x + v.x, y + v.y, z + v.z};
    }
    
    Vector3D operator-(const Vector3D& v) const {
        return {x - v.x, y - v.y, z - v.z};
    }
    
    Vector3D operator*(double s) const {
        return {x * s, y * s, z * s};
    }
    
    double magnitude() const {
        return std::sqrt(x*x + y*y + z*z);
    }
    
    Vector3D normalized() const {
        double mag = magnitude();
        return {x/mag, y/mag, z/mag};
    }
};

struct Body {
    double mass;
    Vector3D position;
    Vector3D velocity;
};

class NBodySimulation {
private:
    const double G = 6.67430e-11;  // Gravitational constant (SI)
    std::vector<Body> bodies;
    double dt;
    
public:
    NBodySimulation(double timestep = 1.0) : dt(timestep) {}
    
    void addBody(const Body& body) {
        bodies.push_back(body);
    }
    
    Vector3D calculateForce(size_t i) {
        Vector3D totalForce = {0, 0, 0};
        
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;
            
            Vector3D r = bodies[j].position - bodies[i].position;
            double distance = r.magnitude();
            
            // F = G * m1 * m2 / r²
            double forceMag = G * bodies[i].mass * bodies[j].mass / (distance * distance);
            Vector3D force = r.normalized() * forceMag;
            
            totalForce = totalForce + force;
        }
        
        return totalForce;
    }
    
    void step() {
        // Calculate forces
        std::vector<Vector3D> forces;
        for (size_t i = 0; i < bodies.size(); ++i) {
            forces.push_back(calculateForce(i));
        }
        
        // Update velocities and positions
        for (size_t i = 0; i < bodies.size(); ++i) {
            // a = F / m
            Vector3D acceleration = forces[i] * (1.0 / bodies[i].mass);
            
            // Update velocity
            bodies[i].velocity = bodies[i].velocity + acceleration * dt;
            
            // Update position
            bodies[i].position = bodies[i].position + bodies[i].velocity * dt;
        }
    }
    
    void simulate(int steps) {
        for (int i = 0; i < steps; ++i) {
            step();
        }
    }
    
    const std::vector<Body>& getBodies() const {
        return bodies;
    }
};

int main() {
    NBodySimulation sim(3600.0);  // 1 hour timestep
    
    // Simplified Sun-Earth system
    Body sun;
    sun.mass = 1.989e30;  // kg
    sun.position = {0, 0, 0};
    sun.velocity = {0, 0, 0};
    
    Body earth;
    earth.mass = 5.972e24;  // kg
    earth.position = {1.496e11, 0, 0};  // 1 AU
    earth.velocity = {0, 29780, 0};     // Orbital velocity
    
    sim.addBody(sun);
    sim.addBody(earth);
    
    // Simulate for 1 year (in hours)
    int stepsPerYear = 8760;
    sim.simulate(stepsPerYear);
    
    auto bodies = sim.getBodies();
    std::cout << "Earth position after 1 year:" << std::endl;
    std::cout << "x: " << bodies[1].position.x << std::endl;
    std::cout << "y: " << bodies[1].position.y << std::endl;
    std::cout << "z: " << bodies[1].position.z << std::endl;
    
    return 0;
}
```

### Wave Equation (1D)
```cpp
#include <vector>
#include <cmath>

class WaveSimulation {
private:
    std::vector<double> u;      // Current displacement
    std::vector<double> uPrev;  // Previous displacement
    int nx;                      // Number of spatial points
    double dx;                   // Spatial step
    double dt;                   // Time step
    double c;                    // Wave speed
    
public:
    WaveSimulation(int numPoints, double length, double waveSpeed, double timestep)
        : nx(numPoints), c(waveSpeed), dt(timestep) {
        dx = length / (nx - 1);
        u.resize(nx, 0.0);
        uPrev.resize(nx, 0.0);
    }
    
    void setInitialCondition(std::function<double(double)> f) {
        for (int i = 0; i < nx; ++i) {
            double x = i * dx;
            u[i] = f(x);
            uPrev[i] = f(x);
        }
    }
    
    void step() {
        std::vector<double> uNext(nx);
        double r = (c * dt / dx) * (c * dt / dx);
        
        // Update interior points
        for (int i = 1; i < nx - 1; ++i) {
            uNext[i] = 2 * u[i] - uPrev[i] + r * (u[i+1] - 2*u[i] + u[i-1]);
        }
        
        // Boundary conditions (fixed ends)
        uNext[0] = 0;
        uNext[nx-1] = 0;
        
        // Update arrays
        uPrev = u;
        u = uNext;
    }
    
    const std::vector<double>& getDisplacement() const {
        return u;
    }
};

int main() {
    int nx = 100;
    double length = 1.0;
    double c = 1.0;
    double dt = 0.01;
    
    WaveSimulation sim(nx, length, c, dt);
    
    // Initial condition: Gaussian pulse
    sim.setInitialCondition([](double x) {
        return std::exp(-100 * (x - 0.5) * (x - 0.5));
    });
    
    // Simulate
    for (int step = 0; step < 100; ++step) {
        sim.step();
    }
    
    auto u = sim.getDisplacement();
    std::cout << "Wave displacement at t = 1.0:" << std::endl;
    for (size_t i = 0; i < u.size(); i += 10) {
        std::cout << "x = " << (i * length / (nx-1)) << ", u = " << u[i] << std::endl;
    }
    
    return 0;
}
```

---

## Data Analysis

### Statistical Functions
```cpp
#include <vector>
#include <cmath>
#include <algorithm>

class Statistics {
public:
    static double mean(const std::vector<double>& data) {
        double sum = 0;
        for (double x : data) sum += x;
        return sum / data.size();
    }
    
    static double variance(const std::vector<double>& data) {
        double m = mean(data);
        double sum = 0;
        for (double x : data) {
            sum += (x - m) * (x - m);
        }
        return sum / data.size();
    }
    
    static double stddev(const std::vector<double>& data) {
        return std::sqrt(variance(data));
    }
    
    static double median(std::vector<double> data) {
        std::sort(data.begin(), data.end());
        size_t n = data.size();
        if (n % 2 == 0) {
            return (data[n/2 - 1] + data[n/2]) / 2.0;
        } else {
            return data[n/2];
        }
    }
    
    static double covariance(const std::vector<double>& x, const std::vector<double>& y) {
        double mx = mean(x);
        double my = mean(y);
        double sum = 0;
        for (size_t i = 0; i < x.size(); ++i) {
            sum += (x[i] - mx) * (y[i] - my);
        }
        return sum / x.size();
    }
    
    static double correlation(const std::vector<double>& x, const std::vector<double>& y) {
        return covariance(x, y) / (stddev(x) * stddev(y));
    }
};

int main() {
    std::vector<double> data = {1.2, 2.3, 3.1, 4.5, 5.2, 6.7, 7.1, 8.9, 9.3, 10.1};
    
    std::cout << "Mean: " << Statistics::mean(data) << std::endl;
    std::cout << "Std Dev: " << Statistics::stddev(data) << std::endl;
    std::cout << "Median: " << Statistics::median(data) << std::endl;
    
    return 0;
}
```

### Linear Regression
```cpp
#include <vector>

struct LinearFit {
    double slope;
    double intercept;
    double r_squared;
};

LinearFit linearRegression(const std::vector<double>& x, const std::vector<double>& y) {
    size_t n = x.size();
    
    // Calculate means
    double mx = 0, my = 0;
    for (size_t i = 0; i < n; ++i) {
        mx += x[i];
        my += y[i];
    }
    mx /= n;
    my /= n;
    
    // Calculate slope
    double num = 0, den = 0;
    for (size_t i = 0; i < n; ++i) {
        num += (x[i] - mx) * (y[i] - my);
        den += (x[i] - mx) * (x[i] - mx);
    }
    double slope = num / den;
    double intercept = my - slope * mx;
    
    // Calculate R²
    double ss_res = 0, ss_tot = 0;
    for (size_t i = 0; i < n; ++i) {
        double y_pred = slope * x[i] + intercept;
        ss_res += (y[i] - y_pred) * (y[i] - y_pred);
        ss_tot += (y[i] - my) * (y[i] - my);
    }
    double r_squared = 1 - (ss_res / ss_tot);
    
    return {slope, intercept, r_squared};
}

int main() {
    std::vector<double> x = {1, 2, 3, 4, 5};
    std::vector<double> y = {2.1, 4.2, 5.9, 8.1, 10.2};
    
    auto fit = linearRegression(x, y);
    
    std::cout << "y = " << fit.slope << " * x + " << fit.intercept << std::endl;
    std::cout << "R² = " << fit.r_squared << std::endl;
    
    return 0;
}
```

---

## Optimization Techniques

### Cache-Friendly Loops
```cpp
#include <vector>
#include <chrono>

// Slow: Column-major access (cache misses)
void slowMatrixMultiply(const std::vector<std::vector<double>>& A,
                        const std::vector<std::vector<double>>& B,
                        std::vector<std::vector<double>>& C) {
    int n = A.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                C[i][j] += A[i][k] * B[k][j];  // B[k][j] not cache-friendly
            }
        }
    }
}

// Fast: Row-major access (cache hits)
void fastMatrixMultiply(const std::vector<std::vector<double>>& A,
                        const std::vector<std::vector<double>>& B,
                        std::vector<std::vector<double>>& C) {
    int n = A.size();
    
    // Transpose B for cache efficiency
    std::vector<std::vector<double>> BT(n, std::vector<double>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            BT[j][i] = B[i][j];
        }
    }
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                C[i][j] += A[i][k] * BT[j][k];  // Both row-major
            }
        }
    }
}
```

### Compiler Optimizations
```cpp
// Enable with compilation flags: -O2 or -O3

// 1. Loop unrolling (manual)
// Slow
for (int i = 0; i < n; ++i) {
    sum += arr[i];
}

// Fast (unrolled)
for (int i = 0; i < n; i += 4) {
    sum += arr[i];
    sum += arr[i+1];
    sum += arr[i+2];
    sum += arr[i+3];
}

// 2. Strength reduction
// Slow
for (int i = 0; i < n; ++i) {
    y = i * 5;  // Multiplication
}

// Fast
y = 0;
for (int i = 0; i < n; ++i) {
    y += 5;  // Addition
}

// 3. Avoid function calls in loops
// Slow
for (int i = 0; i < vec.size(); ++i) {  // size() called every iteration
    // ...
}

// Fast
int n = vec.size();
for (int i = 0; i < n; ++i) {
    // ...
}
```

---

## Monte Carlo Methods

### Random Number Generation
```cpp
#include <random>

class RandomGenerator {
private:
    std::mt19937 gen;
    
public:
    RandomGenerator() : gen(std::random_device{}()) {}
    
    double uniform(double a = 0.0, double b = 1.0) {
        std::uniform_real_distribution<> dis(a, b);
        return dis(gen);
    }
    
    double normal(double mean = 0.0, double stddev = 1.0) {
        std::normal_distribution<> dis(mean, stddev);
        return dis(gen);
    }
    
    int uniformInt(int a, int b) {
        std::uniform_int_distribution<> dis(a, b);
        return dis(gen);
    }
};
```

### Monte Carlo Integration
```cpp
#include <cmath>
#include <functional>

double monteCarloIntegrate(std::function<double(double)> f,
                           double a, double b, int nSamples) {
    RandomGenerator rng;
    double sum = 0;
    
    for (int i = 0; i < nSamples; ++i) {
        double x = rng.uniform(a, b);
        sum += f(x);
    }
    
    return (b - a) * sum / nSamples;
}

int main() {
    // Integrate sin(x) from 0 to π
    auto f = [](double x) { return std::sin(x); };
    
    double result = monteCarloIntegrate(f, 0, M_PI, 1000000);
    std::cout << "Monte Carlo result: " << result << std::endl;
    std::cout << "Analytical result: 2.0" << std::endl;
    
    return 0;
}
```

### Estimate Pi
```cpp
#include <random>

double estimatePi(int nSamples) {
    RandomGenerator rng;
    int inside = 0;
    
    for (int i = 0; i < nSamples; ++i) {
        double x = rng.uniform(-1.0, 1.0);
        double y = rng.uniform(-1.0, 1.0);
        
        if (x*x + y*y <= 1.0) {
            inside++;
        }
    }
    
    return 4.0 * inside / nSamples;
}

int main() {
    std::cout << "Pi estimate (1M samples): " << estimatePi(1000000) << std::endl;
    std::cout << "Actual Pi: " << M_PI << std::endl;
    
    return 0;
}
```

---

## Performance Tips

### 1. Use Appropriate Data Types
```cpp
// Prefer fixed-size types for physics
#include <cstdint>

uint32_t count = 0;      // Exactly 32 bits
double position = 0.0;   // High precision for physics
float velocity = 0.0f;   // Lower precision when acceptable
```

### 2. Vectorization
```cpp
// Modern compilers auto-vectorize simple loops
// Use -O3 -march=native flags

// Good for vectorization
for (int i = 0; i < n; ++i) {
    c[i] = a[i] + b[i];
}

// Avoid function calls and branches in hot loops
```

### 3. Memory Alignment
```cpp
// Aligned memory for SIMD
#include <cstdlib>

double* data = (double*)aligned_alloc(32, n * sizeof(double));
// Use data...
free(data);
```

### 4. Profiling
```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// Code to profile
for (int i = 0; i < 1000000; ++i) {
    // ...
}

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Time: " << duration.count() << " ms" << std::endl;
```

**Continue to OPTIMIZATION_TIPS.md for more performance techniques!**
