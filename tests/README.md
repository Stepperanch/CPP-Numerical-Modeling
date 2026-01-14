# Testing Framework Setup

## Installation

Install Google Test via Homebrew:
```bash
brew install googletest
```

## Running Tests

### Compile and run the example test:
```bash
cd ~/Documents/CPP_Workspace
clang++ -std=c++17 tests/test_example.cpp -lgtest -lgtest_main -o bin/tests
./bin/tests
```

### Expected output:
```
[==========] Running 7 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 3 tests from PhysicsTest
[ RUN      ] PhysicsTest.ProjectileRangeAt45Degrees
[       OK ] PhysicsTest.ProjectileRangeAt45Degrees (0 ms)
[ RUN      ] PhysicsTest.ProjectileRangeAt90Degrees
[       OK ] PhysicsTest.ProjectileRangeAt90Degrees (0 ms)
[ RUN      ] PhysicsTest.GravityEffect
[       OK ] PhysicsTest.GravityEffect (0 ms)
[----------] 3 tests from PhysicsTest (0 ms total)

[----------] 3 tests from MathTest
[ RUN      ] MathTest.FactorialZero
[       OK ] MathTest.FactorialZero (0 ms)
[ RUN      ] MathTest.FactorialPositive
[       OK ] MathTest.FactorialPositive (0 ms)
[ RUN      ] MathTest.FactorialOne
[       OK ] MathTest.FactorialOne (0 ms)
[----------] 3 tests from MathTest (0 ms total)

[----------] 1 test from VectorTest
[ RUN      ] VectorTest.Addition
[       OK ] VectorTest.Addition (0 ms)
[----------] 1 test from VectorTest (0 ms total)

[==========] 7 tests from 3 test suites ran. (0 ms total)
[  PASSED  ] 7 tests.
```

## Writing Your Own Tests

### 1. Create a test file in `tests/` directory

```cpp
#include <gtest/gtest.h>

// Your function to test
double myFunction(double x) {
    return x * x;
}

// Write tests
TEST(MySuite, TestName) {
    EXPECT_EQ(myFunction(2), 4);
    EXPECT_DOUBLE_EQ(myFunction(1.5), 2.25);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 2. Common assertions

```cpp
EXPECT_EQ(a, b);           // a == b
EXPECT_NE(a, b);           // a != b
EXPECT_LT(a, b);           // a < b
EXPECT_LE(a, b);           // a <= b
EXPECT_GT(a, b);           // a > b
EXPECT_GE(a, b);           // a >= b

EXPECT_DOUBLE_EQ(a, b);    // Floating point equality
EXPECT_NEAR(a, b, tol);    // |a - b| <= tol

EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

ASSERT_EQ(a, b);           // Stops test on failure
```

### 3. Test fixtures (for complex setup)

```cpp
class MyTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }
    
    void TearDown() override {
        // Cleanup after each test
    }
    
    // Shared data
    std::vector<double> data;
};

TEST_F(MyTestFixture, TestName) {
    // Use shared data
    data.push_back(1.0);
    EXPECT_EQ(data.size(), 1);
}
```

## Test-Driven Development Workflow

1. Write a test (it will fail - function doesn't exist yet)
2. Write minimal code to make test pass
3. Refactor code while keeping tests passing
4. Repeat

## VS Code Task Integration

Add this to your tasks.json to run tests quickly:

```json
{
    "label": "Run Tests",
    "type": "shell",
    "command": "clang++ -std=c++17 tests/*.cpp -lgtest -lgtest_main -o bin/tests && ./bin/tests",
    "group": "test",
    "problemMatcher": ["$gcc"]
}
```

Then use **Cmd+Shift+P** → "Tasks: Run Test Task" → "Run Tests"
