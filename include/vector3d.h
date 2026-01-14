/**
 * @file vector3d.h
 * @brief 3D vector class for physics calculations
 * @author CPP_Workspace
 * @date 2026-01-07
 */

#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <iostream>

/**
 * @class Vector3D
 * @brief Three-dimensional vector with basic operations
 * 
 * Provides vector arithmetic, magnitude calculation, normalization,
 * and other common operations for 3D vectors used in physics simulations.
 */
class Vector3D {
private:
    double x, y, z;  ///< Vector components

public:
    /**
     * @brief Default constructor - initializes to zero vector
     */
    Vector3D();
    
    /**
     * @brief Parameterized constructor
     * @param x X-component
     * @param y Y-component
     * @param z Z-component
     */
    Vector3D(double x, double y, double z);
    
    // Getters
    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }
    
    // Setters
    void setX(double x) { this->x = x; }
    void setY(double y) { this->y = y; }
    void setZ(double z) { this->z = z; }
    
    /**
     * @brief Calculates the magnitude (length) of the vector
     * @return Vector magnitude
     */
    double magnitude() const;
    
    /**
     * @brief Returns a normalized (unit) vector
     * @return Normalized vector with magnitude 1
     */
    Vector3D normalized() const;
    
    /**
     * @brief Calculates dot product with another vector
     * @param other The other vector
     * @return Dot product result
     */
    double dot(const Vector3D& other) const;
    
    /**
     * @brief Calculates cross product with another vector
     * @param other The other vector
     * @return Cross product vector
     */
    Vector3D cross(const Vector3D& other) const;
    
    /**
     * @brief Vector addition operator
     * @param other Vector to add
     * @return Sum of vectors
     */
    Vector3D operator+(const Vector3D& other) const;
    
    /**
     * @brief Vector subtraction operator
     * @param other Vector to subtract
     * @return Difference of vectors
     */
    Vector3D operator-(const Vector3D& other) const;
    
    /**
     * @brief Scalar multiplication operator
     * @param scalar Scalar value to multiply
     * @return Scaled vector
     */
    Vector3D operator*(double scalar) const;
    
    /**
     * @brief Scalar division operator
     * @param scalar Scalar value to divide by
     * @return Scaled vector
     */
    Vector3D operator/(double scalar) const;
    
    /**
     * @brief Prints vector to output stream
     * @param os Output stream
     */
    void print(std::ostream& os = std::cout) const;
};

#endif  // VECTOR3D_H
