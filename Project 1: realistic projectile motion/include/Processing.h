
#ifndef PROCESSING_H
#define PROCESSING_H

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Projectile.h"

Trajectory rk4Simulation(Projectile& proj, double timeStep, const Vector3D& wind, double maxTime);

void addInfoToStream(std::stringstream& info_stream, const Projectile& proj);

void addInfoToStream2(std::stringstream& info_stream, const Trajectory& trajectory);

class Run {
   public:
    Vector4D initialPos;
    Vector3D initialVel;
    Vector3D initialSpin;
    double timeStep;
    Vector3D wind;
    double maxTime;
    char choice;

    Run();
};

#endif  // PROCESSING_H