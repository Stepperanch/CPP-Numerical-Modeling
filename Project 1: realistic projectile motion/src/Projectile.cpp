/*
 * Projectile.cpp
 *
 * Implementation of Projectile motion class
 */

#include "Projectile.h"

#include <cmath>
#include <fstream>
#include <iostream>
using namespace std;

// ==================== Vector3D Implementation ====================

// Overload the addition operator for Vector3D
Vector3D Vector3D::operator+(const Vector3D& other) const {
    return Vector3D(x + other.x, y + other.y, z + other.z);
}

// Overaoad the subtraction operator for Vector3D
Vector3D Vector3D::operator-(const Vector3D& other) const {
    return Vector3D(x - other.x, y - other.y, z - other.z);
}

// Overaoad the multiplication operator for scalar multiplication
Vector3D Vector3D::operator*(double scalar) const {
    return Vector3D(x * scalar, y * scalar, z * scalar);
}

// Overload the division operator for scalar division
Vector3D Vector3D::operator/(double scalar) const {
    return Vector3D(x / scalar, y / scalar, z / scalar);
}

// Calculate the magnitude of the vector
// Magnitude = sqrt(x^2 + y^2 + z^2)
double Vector3D::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

// Normalize the vector (make its magnitude 1)
Vector3D Vector3D::normalize() const {
    double mag = magnitude();
    if (mag > 0) {
        return Vector3D(x / mag, y / mag, z / mag);
    }
    return Vector3D(0, 0, 0);
}

// Print the vector components to the console
void Vector3D::print() const {
    std::cout << "(" << x << ", " << y << ", " << z << ")";
}

// Write the vector components to a CSV file
void Vector3D::CSVPrint(std::ostream& out) const {
    out << x << "," << y << "," << z;
}

// ==================== Vector4D Implementation ====================
// Overload the addition operator for Vector4D
Vector4D Vector4D::operator+(const Vector4D& other) const {
    return Vector4D(x + other.x, y + other.y, z + other.z, t);
}

// Overload the subtraction operator for Vector4D
Vector4D Vector4D::operator-(const Vector4D& other) const {
    return Vector4D(x - other.x, y - other.y, z - other.z, t);
}

// Overload the multiplication operator for scalar multiplication
Vector4D Vector4D::operator*(double scalar) const {
    return Vector4D(x * scalar, y * scalar, z * scalar, t);
}

// Overload the division operator for scalar division
Vector4D Vector4D::operator/(double scalar) const {
    return Vector4D(x / scalar, y / scalar, z / scalar, t);
}

// Calculate the spacetime magnitude (Minkowski interval)
double Vector4D::spacetimeMagnitude() const {
    return std::sqrt(x * x + y * y + z * z - t * t);
}

// Normalize the spatial components of the vector
Vector4D Vector4D::normalize() const {
    double mag = magnitude();
    if (mag > 0) {
        return Vector4D(x / mag, y / mag, z / mag, t);
    }
    return Vector4D(0, 0, 0, t);
}

// Print the vector components to the console
void Vector4D::print() const {
    std::cout << "(" << x << ", " << y << ", " << z << ", " << t << ")";
}

// Write the vector components to a CSV file
void Vector4D::CSVPrint(std::ostream& out) const {
    out << x << "," << y << "," << z << "," << t;
}

// ==================== Projectile Implementation ====================

// Default constructor for Projectile
Projectile::Projectile()
    : position(0, 0, 0, 0), velocity(0, 0, 0), mass(1.0), radius(0.1), dragCoefficient(0.47) {}

// Parameterized constructor for Projectile
Projectile::Projectile(Vector4D initialPos, Vector3D initialVel, Vector3D initialSpin, double m,
                       double r, double airDensity, double SOverM, double dragCoeff)
    : position(initialPos),
      velocity(initialVel),
      spin(initialSpin),
      mass(m),
      radius(r),
      dragCoefficient(dragCoeff),
      airDensity(airDensity),
      S(SOverM * m) {}

// Setters for Projectile properties
void Projectile::setPosition(const Vector4D& pos) {
    position = pos;
}

void Projectile::setVelocity(const Vector3D& vel) {
    velocity = vel;
}

void Projectile::setMass(double m) {
    mass = m;
}

void Projectile::setRadius(double r) {
    radius = r;
}

void Projectile::setDragCoefficient(double cd) {
    dragCoefficient = cd;
}

void Projectile::setSpin(const Vector3D& spinVec) {
    spin = spinVec;
}

void Projectile::setAirDensity(double density) {
    airDensity = density;
}

void Projectile::setS(double SOverM) {
    S = SOverM * mass;
}

// Set all properties of the Projectile

void Projectile::setAll(Vector4D pos, Vector3D vel, Vector3D spinVec, double m, double r,
                        double density, double SOverM, double dragCoeff) {
    position = pos;
    velocity = vel;
    spin = spinVec;
    mass = m;
    radius = r;
    airDensity = density;
    S = SOverM * m;
    dragCoefficient = dragCoeff;
}

// Getters
Vector4D Projectile::getPosition() const {
    return position;
}

Vector3D Projectile::getVelocity() const {
    return velocity;
}

double Projectile::getTime() const {
    return position.t;
}

double Projectile::getMass() const {
    return mass;
}

double Projectile::getSpeed() const {
    return velocity.magnitude();
}

double Projectile::getHeight() const {
    return position.z;
}

double Projectile::getRadius() const {
    return radius;
}

double Projectile::getRange() const {
    return std::sqrt(position.x * position.x + position.y * position.y);
}

Vector3D Projectile::getSpin() const {
    return spin;
}

double Projectile::getAirDensity() const {
    return airDensity;
}

double Projectile::getDragCoefficient() const {
    return dragCoefficient;
}

double Projectile::getS() const {
    return S;
}

// Physics calculations
//  Calculate acceleration including gravity and air resistance
Vector3D Projectile::calculateAcceleration(const Vector3D& wind) {
    // Gravity (downward in Z direction, no time component)
    Vector3D gravityForce(0, 0, -mass * GRAVITY);

    Vector3D relativeVelocity = velocity - wind;

    // Air resistance (drag)
    double speed = relativeVelocity.magnitude();  // Spatial magnitude only

    Vector3D dragForce(0, 0, 0);
    if (speed > 0) {
        // F_drag = 0.5 * ρ * v² * Cd * A
        double crossSectionalArea = M_PI * radius * radius;
        double dragMagnitude =
            0.5 * airDensity * speed * speed * dragCoefficient * crossSectionalArea;

        // Drag opposes velocity (spatial components only)
        Vector3D dragDirection = relativeVelocity.normalize() * -1.0;
        dragForce = dragDirection * dragMagnitude;
    } else {
        dragForce = Vector3D(0, 0, 0);
    }

    // Magnus force
    Vector3D magnusForce(0, 0, 0);
    // F_magnus = S * (ω x v) / m
    Vector3D magnusCross = Vector3D(spin.y * relativeVelocity.z - spin.z * relativeVelocity.y,
                                    spin.z * relativeVelocity.x - spin.x * relativeVelocity.z,
                                    spin.x * relativeVelocity.y - spin.y * relativeVelocity.x);

    magnusForce = magnusCross * S;

    // cout << spin.x << " " << spin.y << " " << spin.z << " Spin " << endl;
    // cout << velocity.x << " " << velocity.y << " " << velocity.z << " Velocity " << endl;
    // cout << relativeVelocity.x << " " << relativeVelocity.y << " " << relativeVelocity.z
    //      << " Rel Vel " << endl;
    // cout << magnusForce.x << " " << magnusForce.y << " " << magnusForce.z << " Magnus Force "
    //      << endl;
    // cout << gravityForce.x << " " << gravityForce.y << " " << gravityForce.z << " Gravity Force "
    //      << endl;
    // cout << dragForce.x << " " << dragForce.y << " " << dragForce.z << " Drag Force " << endl;

    // F = ma  =>  a = F/m
    Vector3D totalForce = gravityForce + dragForce + magnusForce;
    return totalForce / mass;
}

void Projectile::move(Vector4D pos, Vector3D vel) {
    position = pos;
    velocity = vel;
}

// Utility
bool Projectile::isGrounded() const {
    return position.z <= 0 && velocity.z <= 0;
}

void Projectile::print() const {
    std::cout << "Position: ";
    position.print();
    std::cout << " | Velocity: ";
    velocity.print();
    std::cout << " | Speed: " << getSpeed() << " m/s" << std::endl;
}
//

void Trajectory::print() const {
    for (const auto& point : points) {
        point.print();
        std::cout << std::endl;
    }
}

void Trajectory::CSVPrint(const std::string& filename, const std::string& info) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    file << info << std::endl;
    file << "Time,X,Y,Z" << std::endl;
    for (const auto& point : points) {
        file << point.t << "," << point.x << "," << point.y << "," << point.z << std::endl;
    }

    file.close();
}

// Standalone RK4 integration function

// The RK4 has a error of h^5 per step and h^4 overall, so it is very accurate for small time steps
// may run into problems with very fast spin due to the cycleical natur of that motion which RK4
// struggles with
