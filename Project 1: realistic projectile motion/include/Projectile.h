/*
 * Projectile.h
 *
 * Projectile motion class with realistic physics
 * Includes air resistance, wind, and gravity
 */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Class representing a 3D vector
class Vector3D {
   public:
    double x, y, z;  // x, y, z = spatial coordinates

    // Constructors
    Vector3D() : x(0), y(0), z(0) {}                                    // Default constructor
    Vector3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}  // Parameterized constructor

    // Vector operations (spatial only, time is independent)
    Vector3D operator+(const Vector3D& other) const;  // Vector addition
    Vector3D operator-(const Vector3D& other) const;  // Vector subtraction
    Vector3D operator*(double scalar) const;          // Scalar multiplication
    Vector3D operator/(double scalar) const;          // Scalar division

    // Utility functions
    double magnitude() const;                // Calculate the magnitude of the vector
    Vector3D normalize() const;              // Normalize the vector
    void print() const;                      // Print the vector components
    void CSVPrint(std::ostream& out) const;  // Write the vector to a CSV file
};

// Class representing a 4D vector (extends Vector3D)
class Vector4D : public Vector3D {
   public:
    double t;  // t = time coordinate

    // Constructors
    Vector4D() : Vector3D(), t(0) {}  // Default constructor
    Vector4D(double x_, double y_, double z_, double t_)
        : Vector3D(x_, y_, z_), t(t_) {}  // Parameterized constructor

    // Vector operations (4D)
    Vector4D operator+(const Vector4D& other) const;  // Vector addition
    Vector4D operator-(const Vector4D& other) const;  // Vector subtraction
    Vector4D operator*(double scalar) const;          // Scalar multiplication
    Vector4D operator/(double scalar) const;          // Scalar division

    // Utility functions
    double spacetimeMagnitude() const;       // Calculate the 4D spacetime interval
    Vector4D normalize() const;              // Normalize the spatial components
    void print() const;                      // Print the vector components
    void CSVPrint(std::ostream& out) const;  // Write the vector to a CSV file
};

// Class representing a projectile with realistic physics
class Projectile {
   private:
    Vector4D position;  // Position (x, y, z, t) - (m, m, m, s)
    Vector3D velocity;  // Velocity (vx, vy, vz) - (m/s, m/s, m/s)
    Vector3D spin;      // Spin vector (wx, wy, wz) - (rad/s, rad/s, rad/s)

    double mass;             // Mass (kg)
    double radius;           // Radius (m) - for air resistance
    double dragCoefficient;  // Drag coefficient (dimensionless)
    double airDensity;       // Air density (kg/m³)
    double S;                // Spin factor (m²/s)

    // Environmental constants
    static constexpr double GRAVITY = 9.81;  // Gravitational acceleration (m/s²)

   public:
    // Constructors
    Projectile();  // Default constructor
    Projectile(Vector4D initialPos, Vector3D initialVel, Vector3D initialSpin, double mass,
               double radius, double airDensity, double SOverM,
               double dragCoeff);  // Parameterized constructor

    // Setters
    void setPosition(const Vector4D& pos);  // Set the position
    void setVelocity(const Vector3D& vel);  // Set the velocity
    void setMass(double m);                 // Set the mass
    void setRadius(double r);               // Set the radius
    void setDragCoefficient(double cd);     // Set the drag coefficient
    void setSpin(const Vector3D& spinVec);  // Set the spin vector
    void setAirDensity(double density);     // Set the air density
    void setS(double SOverM);               // Set the spin factor

    void setAll(Vector4D pos, Vector3D vel, Vector3D spinVec, double m, double r, double density,
                double SOverM, double dragCoeff);  // Set all properties

    void move(Vector4D pos, Vector3D vel);  // Update position and velocity

    // Getters
    Vector4D getPosition() const;       // Get the position
    Vector3D getVelocity() const;       // Get the velocity
    double getTime() const;             // Get the time coordinate
    double getMass() const;             // Get the mass
    double getSpeed() const;            // Get the speed (magnitude of velocity)
    double getHeight() const;           // Get the height (z-coordinate)
    double getRange() const;            // Get the horizontal distance from origin
    Vector3D getSpin() const;           // Get the spin vector
    double getAirDensity() const;       // Get the air density
    double getS() const;                // Get the spin factor
    double getRadius() const;           // Get the radius
    double getDragCoefficient() const;  // Get the drag coefficient

    // Physics calculations
    Vector3D calculateAcceleration(const Vector3D& wind = Vector3D(0, 0,
                                                                   0));  // Calculate acceleration

    // Utility
    bool isGrounded() const;  // Check if the projectile is on the ground
    void print() const;       // Print the projectile's state
};

// Projectile(Vector4D initialPos, Vector3D initialVel, Vector3D initialSpin, double mass,
//                double radius, double airDensity, double SOverM,
//                double dragCoeff);
// Class representing a baseball (inherits from Projectile)

class Baseball : public Projectile {
   public:
    Baseball(Vector4D initialPos, Vector3D initialVel, Vector3D initialSpin)
        : Projectile(initialPos, initialVel, initialSpin, 0.149, 0.0366, 1.225, 4.1e-4, 0.35) {
    }  // Initialize with typical baseball properties
};

class pingPongBall : public Projectile {
   public:
    pingPongBall(Vector4D initialPos, Vector3D initialVel, Vector3D initialSpin)
        : Projectile(initialPos, initialVel, initialSpin, 0.0027, 0.02, 1.27, 0.04, 0.5) {
    }  // Initialize with typical ping pong ball properties
};

class perfectProjectile : public Projectile {
   public:
    perfectProjectile(Vector4D initialPos, Vector3D initialVel, Vector3D initialSpin)
        : Projectile(initialPos, initialVel, initialSpin, 1.0, 0.1, 0.0, 0.0, 0.0) {
    }  // No air resistance or spin effects
};

// Class representing a trajectory (collection of points)
class Trajectory {
   private:
    std::vector<Vector4D> points;  // List of points in the trajectory

   public:
    void addPoint(const Vector4D& point) {  // Add a point to the trajectory
        points.push_back(point);
    }

    void print() const;  // Print the trajectory points
    void CSVPrint(const std::string& filename,
                  const std::string& info) const;  // Write the trajectory to a CSV file

    const std::vector<Vector4D>& getPoints() const {
        return points;
    }

    // get the final state of the trajectory
    Vector4D getFinalPoint() const {
        if (!points.empty()) {
            return points.back();
        }
        return Vector4D();
    }
};

class valadationWithoutAirResistance : public Projectile {
   public:
    valadationWithoutAirResistance()
        : Projectile(Vector4D(0, 0, 10, 0), Vector3D(15, 5, 15), Vector3D(0, 0, 0), 1.0, 0.1, 0.0,
                     0.0, 0.0) {}  // No air resistance or spin effects
};

class valadationWithAirResistance : public pingPongBall {
   public:
    valadationWithAirResistance()
        : pingPongBall(Vector4D(0, 0, 10, 0), Vector3D(15, 5, 15), Vector3D(0, 0, 0)) {
    }  // No  spin effects
};

class valadationWithMagnusEffect : public pingPongBall {
   public:
    valadationWithMagnusEffect()
        : pingPongBall(Vector4D(0, 0, 10, 0), Vector3D(15, 5, 15), Vector3D(-20, -40, 20)) {
    }  // with spin effects
};

// final submition
//  Mass of ball: 2.7 grams
//  Diameter of ball: 4.0 centimeters
//  Air density: 1.27 kg/m3
//  Drag coefficient: 0.50
//  Initial position: (0,0,5) meters
//  Initial velocity: (4,4,10) meters/second
//  Initial spin: (-50,-100,100) rad/second

//  Vector4D initialPos, Vector3D initialVel, Vector3D initialSpin, double mass,
//                double radius, double airDensity, double SOverM,
//                double dragCoeff

class finalSubmition : public Projectile {
   public:
    finalSubmition()
        : Projectile(Vector4D(0, 0, 5, 0), Vector3D(4, 4, 10), Vector3D(-50, -100, 100), 0.0027,
                     0.02, 1.27, 0.04, 0.5) {}  // No  spin effects
};

#endif  // PROJECTILE_H
