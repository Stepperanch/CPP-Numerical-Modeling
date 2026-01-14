# Project 1: Realistic Projectile Motion

A physics simulation of projectile motion including air resistance, gravity, wind effects, and spin-induced Magnus force.

## 📁 Project Structure

```
Project 1: realistic projectile motion/
├── main.cpp              # Main simulation program
├── include/
│   └── Projectile.h      # Header file for Projectile, Vector3D, and Vector4D classes
├── src/
│   └── Projectile.cpp    # Implementation of Projectile class
├── bin/                  # Compiled executables (auto-generated)
├── Output/               # Directory for simulation output files (e.g., CSVs)
└── README.md             # This file
```

## 🚀 Quick Start

### Build and Run
```bash
# From VS Code: Press F5

# From terminal:
clang++ -std=c++17 -O2 -I./include main.cpp src/Projectile.cpp -o bin/projectile
./bin/projectile
```

## 🎯 Features

- **Realistic Physics**
  - Gravity (9.81 m/s²)
  - Air resistance (drag force)
  - Wind effects
  - Spin-induced Magnus force
  - Ground collision

- **Vector3D and Vector4D Classes**
  - 3D and 4D vector operations
  - Magnitude and normalization
  - Operator overloading

- **Projectile Class**
  - Position, velocity, acceleration tracking
  - Customizable mass, radius, and drag coefficient
  - Spin effects for Magnus force
  - Real-time simulation updates

- **Runge-Kutta Integration**
  - 4th-order accuracy for smooth trajectories
  - Handles complex forces like drag and Magnus

## 📊 Example Scenarios

1. **Cannonball** - Heavy projectile with high initial velocity
2. **Baseball** - Sports projectile with realistic aerodynamics and spin
3. **Basketball** - Free throw trajectory simulation
4. **Custom** - Define your own projectile parameters

## 🔧 Physics Equations

### Gravity Force
```
F_gravity = -m * g
```

### Drag Force
```
F_drag = -0.5 * ρ * v² * Cd * A * v̂
```
Where:
- ρ = air density (1.225 kg/m³)
- v = velocity magnitude
- Cd = drag coefficient
- A = cross-sectional area (πr²)
- v̂ = velocity unit vector

### Magnus Force
```
F_magnus = S * (ω x v)
```
Where:
- S = spin factor (m²/s)
- ω = spin vector (rad/s)
- v = velocity vector (m/s)

### Integration (Runge-Kutta 4th Order)
```
k1 = f(t, y)
k2 = f(t + dt/2, y + k1 * dt/2)
k3 = f(t + dt/2, y + k2 * dt/2)
k4 = f(t + dt, y + k3 * dt)
y(t+dt) = y(t) + (dt/6) * (k1 + 2*k2 + 2*k3 + k4)
```

## 📝 TODO / Extensions

- [x] Add Runge-Kutta integration for better accuracy
- [x] Implement spin effects (Magnus force)
- [ ] Add variable air density with altitude
- [ ] Create trajectory visualization
- [ ] Export data to CSV for plotting
- [ ] Add multiple projectiles simulation
- [ ] Implement wind gusts
- [ ] Add terminal velocity calculations

## 🎓 Learning Objectives

- Object-oriented programming in C++
- Physics simulation techniques
- Numerical integration methods
- Vector mathematics
- Class design and implementation
- Header/source file organization

---

**Press F5 to compile and run!**
