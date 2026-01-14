/**
 * @file example_multi_file.cpp
 * @brief Example demonstrating header/source file split
 * 
 * This shows how to use the Vector3D class from separate header/source files.
 * 
 * To compile:
 *   clang++ -std=c++17 example_multi_file.cpp src/vector3d.cpp -o bin/example
 * 
 * Or use the Makefile:
 *   Update SOURCES in Makefile to include both files, then run: make
 */

#include <iostream>
#include <iomanip>
#include "include/vector3d.h"

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  Vector3D Class Example" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;
    
    // Create vectors
    Vector3D v1(3.0, 4.0, 0.0);
    Vector3D v2(1.0, 0.0, 0.0);
    
    std::cout << "Vector 1: ";
    v1.print();
    std::cout << std::endl;
    
    std::cout << "Vector 2: ";
    v2.print();
    std::cout << std::endl;
    std::cout << std::endl;
    
    // Magnitude
    std::cout << "Magnitude of v1: " << v1.magnitude() << std::endl;
    std::cout << std::endl;
    
    // Normalized vector
    Vector3D v1_norm = v1.normalized();
    std::cout << "Normalized v1: ";
    v1_norm.print();
    std::cout << " (magnitude: " << v1_norm.magnitude() << ")" << std::endl;
    std::cout << std::endl;
    
    // Vector operations
    Vector3D sum = v1 + v2;
    std::cout << "v1 + v2 = ";
    sum.print();
    std::cout << std::endl;
    
    Vector3D diff = v1 - v2;
    std::cout << "v1 - v2 = ";
    diff.print();
    std::cout << std::endl;
    
    Vector3D scaled = v1 * 2.0;
    std::cout << "v1 * 2 = ";
    scaled.print();
    std::cout << std::endl;
    std::cout << std::endl;
    
    // Dot product
    double dotProduct = v1.dot(v2);
    std::cout << "v1 · v2 = " << dotProduct << std::endl;
    std::cout << std::endl;
    
    // Cross product
    Vector3D v3(0.0, 1.0, 0.0);
    Vector3D cross = v2.cross(v3);
    std::cout << "v2 × v3 = ";
    cross.print();
    std::cout << std::endl;
    std::cout << std::endl;
    
    std::cout << "✓ Header/Source split working!" << std::endl;
    
    return 0;
}
