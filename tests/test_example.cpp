/*
 * Example Test File using Google Test
 * 
 * To use this:
 * 1. Install Google Test: brew install googletest
 * 2. Compile: clang++ -std=c++17 test_example.cpp -lgtest -lgtest_main -o ../bin/tests
 * 3. Run: ../bin/tests
 */

#include <gtest/gtest.h>
#include <cmath>

// Example: Function to test
double calculateRange(double v0, double angle, double g = 9.81) {
    return (v0 * v0 * std::sin(2 * angle)) / g;
}

double factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// Test Suite: Physics calculations
TEST(PhysicsTest, ProjectileRangeAt45Degrees) {
    double v0 = 10.0;  // m/s
    double angle = M_PI / 4;  // 45 degrees
    double range = calculateRange(v0, angle);
    
    EXPECT_NEAR(range, 10.19, 0.1);  // Expected range ~10.19m
}

TEST(PhysicsTest, ProjectileRangeAt90Degrees) {
    double v0 = 10.0;
    double angle = M_PI / 2;  // Straight up
    double range = calculateRange(v0, angle);
    
    EXPECT_NEAR(range, 0.0, 0.01);  // Should be near zero
}

TEST(PhysicsTest, GravityEffect) {
    double v0 = 10.0;
    double angle = M_PI / 4;
    
    double rangeEarth = calculateRange(v0, angle, 9.81);
    double rangeMoon = calculateRange(v0, angle, 1.62);
    
    EXPECT_GT(rangeMoon, rangeEarth);  // Moon range should be greater
}

// Test Suite: Math operations
TEST(MathTest, FactorialZero) {
    EXPECT_EQ(factorial(0), 1);
}

TEST(MathTest, FactorialPositive) {
    EXPECT_EQ(factorial(5), 120);
    EXPECT_EQ(factorial(3), 6);
}

TEST(MathTest, FactorialOne) {
    EXPECT_EQ(factorial(1), 1);
}

// Test Suite: Vector operations (example)
TEST(VectorTest, Addition) {
    struct Vector2D {
        double x, y;
        Vector2D operator+(const Vector2D& v) const {
            return {x + v.x, y + v.y};
        }
    };
    
    Vector2D v1{1.0, 2.0};
    Vector2D v2{3.0, 4.0};
    Vector2D result = v1 + v2;
    
    EXPECT_DOUBLE_EQ(result.x, 4.0);
    EXPECT_DOUBLE_EQ(result.y, 6.0);
}

// Main function (not needed if you link with -lgtest_main)
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
