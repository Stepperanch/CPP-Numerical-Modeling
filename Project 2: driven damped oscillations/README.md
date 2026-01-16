# Project 2: Driven Damped Oscillations

Skeleton project for a driven damped oscillator simulation. You can fill in the physics and numerics.

## Structure
- `main.cpp` — entry point; wire up simulation and I/O.
- `include/oscillator.h` — declarations for oscillator model and helpers.
- `src/oscillator.cpp` — definitions; implement the model here.
- `Output/` — place output data/plots; a `.gitkeep` is included to keep the folder tracked.

## Build (example)
```bash
clang++ -std=c++17 -Wall -Wextra -O2 -g -I./include main.cpp src/oscillator.cpp -o bin/oscillator
```

Feel free to adjust the build to your workflow (CMake/Make/etc.).