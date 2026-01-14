/*
 * Processing.cpp
 *
 * Implementation of Processing class
 */

#include "Processing.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#ifndef _WIN32
#include <unistd.h>
#endif
using namespace std;

// Limitations and error on RK4 method:
// The RK4 method assumes that the acceleration is constant over the time step.
// If the time step is too large, this assumption breaks down, leading to inaccuracies.
// Rapidly changing forces (e.g., due to high drag or spin) can introduce errors.
// To mitigate these issues, use smaller time steps for higher accuracy, especially in scenarios with significant forces.
// However, smaller time steps increase computational load, so a balance must be struck based on the specific simulation requirements.
// Overall, RK4 is very accurate for smooth, continuous forces but may struggle with abrupt changes unless time steps are sufficiently small.


// Standalone RK4 integration function

Trajectory rk4Simulation(Projectile& proj, double timeStep, const Vector3D& wind, double maxTime) {
    Trajectory trajectory;
    trajectory.addPoint(proj.getPosition());

    while (!proj.isGrounded() && proj.getTime() < maxTime) {
        // Save current state
        Vector4D pos0 = proj.getPosition();
        Vector3D vel0 = proj.getVelocity();

        // k1: acceleration and velocity at current state
        Vector3D k1_a = proj.calculateAcceleration(wind);
        Vector3D k1_v = k1_a * timeStep;
        Vector3D k1_x = vel0 * timeStep;

        // k2: acceleration at midpoint using k1
        Vector3D vel_mid1 = vel0 + k1_v * 0.5;
        proj.setVelocity(vel_mid1);
        Vector3D k2_a = proj.calculateAcceleration(wind);
        Vector3D k2_v = k2_a * timeStep;
        Vector3D k2_x = vel_mid1 * timeStep;

        // k3: acceleration at midpoint using k2
        Vector3D vel_mid2 = vel0 + k2_v * 0.5;
        proj.setVelocity(vel_mid2);
        Vector3D k3_a = proj.calculateAcceleration(wind);
        Vector3D k3_v = k3_a * timeStep;
        Vector3D k3_x = vel_mid2 * timeStep;

        // k4: acceleration at endpoint using k3
        Vector3D vel_end = vel0 + k3_v;
        proj.setVelocity(vel_end);
        Vector3D k4_a = proj.calculateAcceleration(wind);
        Vector3D k4_v = k4_a * timeStep;
        Vector3D k4_x = vel_end * timeStep;

        // Weighted average of slopes
        Vector3D new_vel = vel0 + (k1_v + k2_v * 2.0 + k3_v * 2.0 + k4_v) / 6.0;
        Vector3D delta_r = (k1_x + k2_x * 2.0 + k3_x * 2.0 + k4_x) / 6.0;

        // Update position with time
        Vector4D new_pos(pos0.x + delta_r.x, pos0.y + delta_r.y, pos0.z + delta_r.z,
                         pos0.t + timeStep);

        proj.move(new_pos, new_vel);

        // Ground collision check
        if (new_pos.z < 0) {
            new_pos.z = 0;
            Vector3D stopped_vel(0, 0, 0);
            proj.move(new_pos, stopped_vel);
            break;
        }

        trajectory.addPoint(proj.getPosition());
    }

    return trajectory;
}

void addInfoToStream(std::stringstream& info_stream, const Projectile& proj) {
    info_stream << "#Initial Position (m): (" << proj.getPosition().x << ", "
                << proj.getPosition().y << ", " << proj.getPosition().z << ")" << std::endl
                << "#Initial Velocity (m/s): (" << proj.getVelocity().x << ", "
                << proj.getVelocity().y << ", " << proj.getVelocity().z << ")" << std::endl
                << "#Initial Spin (rad/s): (" << proj.getSpin().x << ", " << proj.getSpin().y
                << ", " << proj.getSpin().z << ")" << std::endl
                << "#Diameter (m): " << (proj.getRadius() * 2.0) << std::endl
                << "#Mass (kg): " << proj.getMass() << std::endl
                << "#Drag Coefficient: " << proj.getDragCoefficient() << std::endl
                << "#Air Density (kg/m^3): " << proj.getAirDensity() << std::endl;
}

void addInfoToStream2(std::stringstream& info_stream, const Trajectory& trajectory) {
    info_stream << "#Final Time (s): " << trajectory.getFinalPoint().t << std::endl
                << "#Final Position (m): (" << trajectory.getFinalPoint().x << ", "
                << trajectory.getFinalPoint().y << ", " << trajectory.getFinalPoint().z << ")"
                << std::endl;
}

Run::Run() {
    std::cout << "Realistic Projectile Motion Simulation" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Note: All units are in SI (meters, seconds, m/s, etc.)" << std::endl << std::endl;

    std::cout << "Would you like to" << std::endl;
    std::cout << "1. Run the program to validate the model" << std::endl;
    std::cout << "2. Run a custom simulation" << std::endl;
    std::cout << "3. Run a preset simulation" << std::endl;

    int mode;
    std::cin >> mode;

    Trajectory trajectory;

    std::stringstream info_stream;
    info_stream << "#Projectile Motion Simulation Data" << std::endl;

    switch (mode) {
        case 1: {
            std::cout << "Validation mode selected." << std::endl;
            std::cout << "Choose validation type" << std::endl;
            std::cout << "1. Without air resistance" << std::endl;
            std::cout << "2. With air resistance" << std::endl;
            std::cout << "3. With Magnus effect" << std::endl;
            std::cout << "4. Final Submission" << std::endl;
            int validationType;
            std::cin >> validationType;
            double maxTime = 10.0;
            double timeStep = 0.001;
            Vector3D wind(0, 0, 0);
            switch (validationType) {
                case 1: {
                    valadationWithoutAirResistance valadation;
                    info_stream << "#Validation Type: Without Air Resistance" << std::endl;
                    addInfoToStream(info_stream, valadation);

                    trajectory = rk4Simulation(valadation, timeStep, wind, maxTime);

                    addInfoToStream2(info_stream, trajectory);
                    break;
                }
                case 2: {
                    valadationWithAirResistance valadation;
                    info_stream << "#Validation Type: With Air Resistance" << std::endl;
                    addInfoToStream(info_stream, valadation);

                    trajectory = rk4Simulation(valadation, timeStep, wind, maxTime);

                    addInfoToStream2(info_stream, trajectory);
                    break;
                }
                case 3: {
                    valadationWithMagnusEffect valadation;
                    info_stream << "#Validation Type: With Magnus Effect" << std::endl;
                    addInfoToStream(info_stream, valadation);

                    trajectory = rk4Simulation(valadation, timeStep, wind, maxTime);

                    addInfoToStream2(info_stream, trajectory);
                    break;
                }
                case 4: {
                    finalSubmition valadation;
                    info_stream << "#Validation Type: With Magnus Effect" << std::endl;
                    addInfoToStream(info_stream, valadation);

                    trajectory = rk4Simulation(valadation, timeStep, wind, maxTime);

                    addInfoToStream2(info_stream, trajectory);

                    break;
                }
                default: {
                    std::cout << "Invalid choice. Exiting." << std::endl;
                    return;
                }
            }
            break;
        }
        case 2: {
            std::cout << "Custom simulation mode selected." << std::endl;
            // Get initial position from the user
            std::cout << "Enter initial position (x y z in meters): ";
            double x, y, z;
            std::cin >> x >> y >> z;
            initialPos = Vector4D(x, y, z, 0);  // t=0 at launch

            // Get initial velocity from the user
            std::cout << "Enter initial velocity (vx vy vz in meters per second): ";
            double vx, vy, vz;
            std::cin >> vx >> vy >> vz;
            initialVel = Vector3D(vx, vy, vz);

            // Get initial spin from the user
            std::cout << "Enter initial spin (wx wy wz in radians per second): ";
            double wx, wy, wz;
            std::cin >> wx >> wy >> wz;
            initialSpin = Vector3D(wx, wy, wz);

            // Get time step for the simulation
            std::cout << "Enter time step for simulation (in seconds): ";
            std::cin >> timeStep;

            // Get wind velocity from the user
            std::cout << "Enter wind velocity (wx wy wz in meters per second): ";
            double wind_x, wind_y, wind_z;
            std::cin >> wind_x >> wind_y >> wind_z;
            wind = Vector3D(wind_x, wind_y, wind_z);

            std::cout << "Enter the diameter of the projectile (in meters): ";
            double diameter;
            std::cin >> diameter;
            double radius = diameter / 2.0;

            std::cout << "Enter the mass of the projectile (in kilograms): ";
            double mass;
            std::cin >> mass;

            std::cout << "Enter the drag coefficient of the projectile (dimensionless): ";
            double dragCoeff;
            std::cin >> dragCoeff;

            std::cout << "Enter air density (in kg/m^3): ";
            double airDensity;
            std::cin >> airDensity;

            std::cout << "Enter spin factor S over M (in m^2/s): ";
            double SOverM;
            std::cin >> SOverM;

            std::cout << "Enter maximum simulation time (in seconds): ";
            std::cin >> maxTime;

            // Create a Projectile object with the given parameters
            Projectile customProj(initialPos, initialVel, initialSpin, mass, radius, airDensity,
                                  SOverM, dragCoeff);

            info_stream << "#Custom Simulation" << std::endl;
            addInfoToStream(info_stream, customProj);

            trajectory = rk4Simulation(customProj, timeStep, wind, maxTime);

            addInfoToStream2(info_stream, trajectory);
            break;
        }
        case 3: {
            std::cout << "Preset simulation mode selected." << std::endl;
            std::cout << "Choose preset:" << std::endl;
            std::cout << "1. Ping Pong Ball" << std::endl;
            std::cout << "2. Baseball" << std::endl;
            int presetType;
            std::cin >> presetType;
            double maxTime = 10.0;
            double timeStep = 0.001;
            Vector3D wind(0, 0, 0);
            switch (presetType) {
                case 1: {
                    pingPongBall pingPong(Vector4D(0, 0, 1, 0), Vector3D(10, 10, 10),
                                          Vector3D(0, 0, 50));
                    info_stream << "#Preset: Ping Pong Ball" << std::endl;
                    addInfoToStream(info_stream, pingPong);

                    trajectory = rk4Simulation(pingPong, timeStep, wind, maxTime);

                    addInfoToStream2(info_stream, trajectory);
                    break;
                }
                case 2: {
                    Baseball baseball(Vector4D(0, 0, 1, 0), Vector3D(30, 30, 30),
                                      Vector3D(0, 0, 100));
                    info_stream << "#Preset: Baseball" << std::endl;
                    addInfoToStream(info_stream, baseball);

                    trajectory = rk4Simulation(baseball, timeStep, wind, maxTime);

                    addInfoToStream2(info_stream, trajectory);
                    break;
                }
            }
            break;
        }
    }

    // get the current directory
    char buffer[FILENAME_MAX];
    std::string dir;
    getcwd(buffer, FILENAME_MAX);
    dir = std::string(buffer) + "/Output/";
    string filename = dir + "trajectory" + to_string(1) + ".csv";

    // Iterate through filenames to avoid overwriting existing files
    while (ifstream(filename)) {
        static int fileIndex = 2;
        filename = dir + "trajectory" + to_string(fileIndex) + ".csv";
        fileIndex++;
    }

    trajectory.CSVPrint(filename, info_stream.str());
    cout << "Trajectory data saved to: " << filename << endl;

    // Extract trajectory number from filename for plotting script
    size_t trajPos = filename.find("trajectory");
    size_t dotPos = filename.find_last_of('.');
    string trajectoryNum = filename.substr(
        trajPos + 10, dotPos - trajPos - 10);  // Extract number from "trajectory#.csv"

    // run the plotting script
    string plotCommand = "cd \"" + dir +
                         "..\" && /Users/nelsbuhrley/math-env/bin/python ploting.py <<< \"" +
                         trajectoryNum + "\"";
    system(plotCommand.c_str());

    cout << "Plotting complete." << endl;
}
