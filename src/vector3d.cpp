/**
 * @file vector3d.cpp
 * @brief Implementation of Vector3D class
 */

#include "../include/vector3d.h"
#include <cmath>
#include <iostream>

// Constructors
Vector3D::Vector3D() : x(0), y(0), z(0) {}

Vector3D::Vector3D(double x, double y, double z) : x(x), y(y), z(z) {}

// Magnitude
double Vector3D::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

// Normalized vector
Vector3D Vector3D::normalized() const {
    double mag = magnitude();
    if (mag == 0) {
        return Vector3D(0, 0, 0);
    }
    return Vector3D(x / mag, y / mag, z / mag);
}

// Dot product
double Vector3D::dot(const Vector3D& other) const {
    return x * other.x + y * other.y + z * other.z;
}

// Cross product
Vector3D Vector3D::cross(const Vector3D& other) const {
    return Vector3D(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

// Addition
Vector3D Vector3D::operator+(const Vector3D& other) const {
    return Vector3D(x + other.x, y + other.y, z + other.z);
}

// Subtraction
Vector3D Vector3D::operator-(const Vector3D& other) const {
    return Vector3D(x - other.x, y - other.y, z - other.z);
}

// Scalar multiplication
Vector3D Vector3D::operator*(double scalar) const {
    return Vector3D(x * scalar, y * scalar, z * scalar);
}

// Scalar division
Vector3D Vector3D::operator/(double scalar) const {
    return Vector3D(x / scalar, y / scalar, z / scalar);
}

// Print
void Vector3D::print(std::ostream& os) const {
    os << "(" << x << ", " << y << ", " << z << ")";
}
