# C++ Performance Optimization Guide

## 📚 Table of Contents
1. [Compiler Optimizations](#compiler-optimizations)
2. [Memory Management](#memory-management)
3. [Algorithm Optimization](#algorithm-optimization)
4. [Profiling & Benchmarking](#profiling--benchmarking)
5. [Parallel Computing](#parallel-computing)
6. [Best Practices](#best-practices)

---

## Compiler Optimizations

### Optimization Flags
```bash
# No optimization (fast compile, slow execution)
clang++ -O0 program.cpp -o program

# Basic optimization
clang++ -O1 program.cpp -o program

# Recommended for most cases (balanced)
clang++ -O2 program.cpp -o program

# Aggressive optimization (best performance)
clang++ -O3 program.cpp -o program

# Size optimization (embedded systems)
clang++ -Os program.cpp -o program

# Native CPU optimization
clang++ -O3 -march=native program.cpp -o program

# Link-time optimization
clang++ -O3 -flto program.cpp -o program

# Fast math (may break IEEE 754 compliance)
clang++ -O3 -ffast-math program.cpp -o program
```

### Inline Functions
```cpp
// Compiler hint to inline function (eliminate call overhead)
inline double square(double x) {
    return x * x;
}

// Force inline (use sparingly)
__attribute__((always_inline)) inline int fastAdd(int a, int b) {
    return a + b;
}

// Prevent inlining
__attribute__((noinline)) void debugFunction() {
    // Useful for profiling specific functions
}
```

### Constexpr (Compile-Time Computation)
```cpp
// Computed at compile time
constexpr double PI = 3.14159265359;

constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

// Usage
constexpr int fact5 = factorial(5);  // Computed at compile time
int arr[factorial(4)];               // Array size known at compile time

// C++14: More complex constexpr
constexpr double power(double base, int exp) {
    double result = 1.0;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

constexpr double x = power(2.0, 10);  // 1024.0 at compile time
```

### Attributes
```cpp
// Likely/unlikely branches (C++20)
if (x > 0) [[likely]] {
    // Most common path
} else [[unlikely]] {
    // Rare error case
}

// No discard (warn if return value ignored)
[[nodiscard]] int important() {
    return 42;
}

// Maybe unused (suppress warnings)
void function([[maybe_unused]] int param) {
    // param might not be used
}
```

---

## Memory Management

### Stack vs Heap
```cpp
// Stack: Fast, automatic cleanup, limited size
void stackAllocation() {
    int arr[1000];  // Stack
    // Automatically freed when function returns
}

// Heap: Slower, manual cleanup, larger size
void heapAllocation() {
    int* arr = new int[1000000];  // Heap
    // ... use arr ...
    delete[] arr;  // Must manually free
}

// Modern: Smart pointers (heap with automatic cleanup)
void smartPointers() {
    auto arr = std::make_unique<int[]>(1000000);
    // Automatically freed when arr goes out of scope
}
```

### Memory Layout
```cpp
#include <vector>

// Bad: Vector of vectors (non-contiguous memory)
std::vector<std::vector<int>> matrix(100, std::vector<int>(100));

// Good: Flat 1D array (contiguous memory, cache-friendly)
std::vector<int> flatMatrix(100 * 100);
// Access element (i, j): flatMatrix[i * 100 + j]

// Even better: Use reserve to avoid reallocations
std::vector<int> data;
data.reserve(1000000);  // Pre-allocate space
for (int i = 0; i < 1000000; ++i) {
    data.push_back(i);  // No reallocation
}
```

### Memory Alignment
```cpp
#include <cstdlib>

// Standard allocation (may not be aligned)
double* data1 = new double[1000];

// Aligned allocation for SIMD (32-byte alignment for AVX)
double* data2 = static_cast<double*>(aligned_alloc(32, 1000 * sizeof(double)));
// Use data2...
free(data2);

// Check alignment
bool isAligned = (reinterpret_cast<uintptr_t>(data2) % 32 == 0);

// Struct alignment
struct alignas(64) CacheLinePadded {
    double data[8];  // 64 bytes = 1 cache line
};
```

### Memory Pool
```cpp
template <typename T, size_t PoolSize>
class MemoryPool {
private:
    T pool[PoolSize];
    bool used[PoolSize] = {false};
    
public:
    T* allocate() {
        for (size_t i = 0; i < PoolSize; ++i) {
            if (!used[i]) {
                used[i] = true;
                return &pool[i];
            }
        }
        return nullptr;  // Pool exhausted
    }
    
    void deallocate(T* ptr) {
        size_t index = ptr - pool;
        if (index < PoolSize) {
            used[index] = false;
        }
    }
};

// Usage
MemoryPool<int, 1000> pool;
int* x = pool.allocate();
*x = 42;
pool.deallocate(x);
```

---

## Algorithm Optimization

### Loop Optimization

#### Loop Unrolling
```cpp
// Original
for (int i = 0; i < n; ++i) {
    sum += arr[i];
}

// Unrolled (fewer loop overhead)
int i = 0;
for (; i + 3 < n; i += 4) {
    sum += arr[i];
    sum += arr[i+1];
    sum += arr[i+2];
    sum += arr[i+3];
}
// Handle remainder
for (; i < n; ++i) {
    sum += arr[i];
}
```

#### Loop Fusion
```cpp
// Before: Two separate loops
for (int i = 0; i < n; ++i) {
    a[i] = b[i] + c[i];
}
for (int i = 0; i < n; ++i) {
    d[i] = a[i] * 2;
}

// After: Single loop (better cache usage)
for (int i = 0; i < n; ++i) {
    a[i] = b[i] + c[i];
    d[i] = a[i] * 2;
}
```

#### Loop Interchange (Cache Optimization)
```cpp
// Bad: Column-major access (cache misses)
for (int j = 0; j < cols; ++j) {
    for (int i = 0; i < rows; ++i) {
        matrix[i][j] = 0;  // Jumps through memory
    }
}

// Good: Row-major access (cache hits)
for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
        matrix[i][j] = 0;  // Sequential memory access
    }
}
```

### Data Structure Selection
```cpp
// Use case: Frequent insertions at front
// Bad: std::vector (O(n) insertions)
std::vector<int> vec;
vec.insert(vec.begin(), 42);  // Slow

// Good: std::deque (O(1) insertions)
std::deque<int> deq;
deq.push_front(42);  // Fast

// Use case: Frequent lookups
// Bad: std::vector with linear search (O(n))
auto it = std::find(vec.begin(), vec.end(), 42);

// Good: std::unordered_set (O(1) average)
std::unordered_set<int> set;
bool found = set.count(42) > 0;

// Use case: Sorted data with lookups
// Best: std::set or std::map (O(log n))
std::set<int> sortedSet;
bool exists = sortedSet.count(42) > 0;
```

### Avoiding Unnecessary Copies
```cpp
// Bad: Copies vector
void processVector(std::vector<int> vec) {
    // ...
}

// Good: Pass by const reference
void processVector(const std::vector<int>& vec) {
    // ...
}

// For modification: Pass by reference
void modifyVector(std::vector<int>& vec) {
    // ...
}

// Move semantics (C++11)
std::vector<int> createVector() {
    std::vector<int> vec(1000000);
    // ... fill vec ...
    return vec;  // No copy, vec is moved
}

auto data = createVector();  // Move, not copy
```

### String Optimization
```cpp
// Bad: Repeated concatenation (creates many temporaries)
std::string result = "";
for (int i = 0; i < 1000; ++i) {
    result += "x";  // Reallocates each time
}

// Good: Reserve space first
std::string result;
result.reserve(1000);
for (int i = 0; i < 1000; ++i) {
    result += "x";  // No reallocation
}

// Best: Use stringstream for complex concatenation
std::ostringstream oss;
for (int i = 0; i < 1000; ++i) {
    oss << "x";
}
std::string result = oss.str();

// Use string_view to avoid copies (C++17)
void processString(std::string_view str) {
    // No copy of string
}
```

---

## Profiling & Benchmarking

### Basic Timing
```cpp
#include <chrono>

class Timer {
private:
    std::chrono::high_resolution_clock::time_point start;
    
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }
    
    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }
};

// Usage
Timer timer;
// ... code to measure ...
std::cout << "Time: " << timer.elapsed() << " seconds" << std::endl;
```

### Benchmarking Template
```cpp
template <typename Func>
double benchmark(Func f, int iterations = 1000) {
    Timer timer;
    
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    
    return timer.elapsed() / iterations;
}

// Usage
auto avgTime = benchmark([]() {
    std::vector<int> vec(10000);
    std::sort(vec.begin(), vec.end());
});

std::cout << "Average time: " << avgTime << " seconds" << std::endl;
```

### macOS Instruments Profiling
```bash
# Compile with debug symbols
clang++ -O2 -g program.cpp -o program

# Run with Instruments (Time Profiler)
instruments -t "Time Profiler" ./program

# Or use command-line profiling
# 1. Sample while running
sample program 10

# 2. Analyze with profiler
# Xcode -> Open Developer Tool -> Instruments
```

### Profiling with gprof
```bash
# Compile with profiling
clang++ -O2 -pg program.cpp -o program

# Run program (generates gmon.out)
./program

# Generate report
gprof program gmon.out > analysis.txt
```

### Memory Profiling (Valgrind alternative on macOS)
```bash
# Use leaks tool
leaks --atExit -- ./program

# Or use Address Sanitizer
clang++ -O1 -g -fsanitize=address program.cpp -o program
./program
```

---

## Parallel Computing

### C++11 Threads
```cpp
#include <thread>
#include <vector>

void worker(int id) {
    std::cout << "Thread " << id << " working" << std::endl;
}

int main() {
    std::vector<std::thread> threads;
    
    // Launch threads
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker, i);
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

### Parallel Sum Example
```cpp
#include <thread>
#include <vector>
#include <numeric>

double parallelSum(const std::vector<double>& data, int numThreads = 4) {
    std::vector<std::thread> threads;
    std::vector<double> partialSums(numThreads, 0);
    
    size_t chunkSize = data.size() / numThreads;
    
    auto worker = [&](int id) {
        size_t start = id * chunkSize;
        size_t end = (id == numThreads - 1) ? data.size() : start + chunkSize;
        
        partialSums[id] = std::accumulate(data.begin() + start,
                                         data.begin() + end, 0.0);
    };
    
    // Launch threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker, i);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Combine results
    return std::accumulate(partialSums.begin(), partialSums.end(), 0.0);
}
```

### Mutex for Thread Safety
```cpp
#include <thread>
#include <mutex>
#include <vector>

class ThreadSafeCounter {
private:
    int count = 0;
    std::mutex mtx;
    
public:
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        count++;
    }
    
    int getCount() const {
        return count;
    }
};

int main() {
    ThreadSafeCounter counter;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 1000; ++j) {
                counter.increment();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Count: " << counter.getCount() << std::endl;  // 10000
    
    return 0;
}
```

### OpenMP (Easier Parallelization)
```cpp
#include <omp.h>

// Compile: clang++ -Xpreprocessor -fopenmp -lomp program.cpp

int main() {
    const int n = 1000000;
    std::vector<double> data(n);
    
    // Parallel for loop
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        data[i] = i * i;
    }
    
    // Parallel reduction
    double sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < n; ++i) {
        sum += data[i];
    }
    
    std::cout << "Sum: " << sum << std::endl;
    
    return 0;
}
```

---

## Best Practices

### 1. Prefer STL Algorithms
```cpp
// Bad: Manual loop
int sum = 0;
for (size_t i = 0; i < vec.size(); ++i) {
    sum += vec[i];
}

// Good: STL algorithm (compiler can optimize)
int sum = std::accumulate(vec.begin(), vec.end(), 0);

// Sorting
std::sort(vec.begin(), vec.end());

// Finding
auto it = std::find(vec.begin(), vec.end(), 42);

// Transforming
std::transform(vec.begin(), vec.end(), vec.begin(),
               [](int x) { return x * 2; });
```

### 2. Move Semantics
```cpp
// Return by value (uses move)
std::vector<int> createLargeVector() {
    std::vector<int> vec(1000000);
    return vec;  // Move, not copy
}

// Perfect forwarding in templates
template <typename T>
void wrapper(T&& arg) {
    function(std::forward<T>(arg));
}
```

### 3. Avoid Premature Optimization
```cpp
// 1. Write clear, correct code first
// 2. Profile to find bottlenecks
// 3. Optimize only the slow parts
// 4. Verify optimization improved performance

// Example: Don't micro-optimize this
for (int i = 0; i < 10; ++i) {
    std::cout << i << std::endl;
}

// But do optimize this (if profiling shows it's slow)
for (int i = 0; i < 10000000; ++i) {
    // Heavy computation
}
```

### 4. Compiler Warnings
```bash
# Enable all warnings
clang++ -Wall -Wextra -Wpedantic program.cpp

# Treat warnings as errors
clang++ -Werror program.cpp

# Additional useful warnings
clang++ -Wall -Wextra -Wshadow -Wconversion program.cpp
```

### 5. Use const Correctness
```cpp
class Vector2D {
private:
    double x, y;
    
public:
    // Const member functions (don't modify object)
    double getX() const { return x; }
    double getY() const { return y; }
    double magnitude() const { return std::sqrt(x*x + y*y); }
    
    // Non-const member functions (can modify)
    void setX(double newX) { x = newX; }
    void setY(double newY) { y = newY; }
};

// Const parameters
void printVector(const Vector2D& vec) {
    // Can only call const methods
    std::cout << vec.getX() << ", " << vec.getY() << std::endl;
}
```

### 6. RAII Pattern
```cpp
// Bad: Manual resource management
FILE* file = fopen("data.txt", "r");
// ... use file ...
fclose(file);  // Easy to forget!

// Good: RAII (automatic cleanup)
{
    std::ifstream file("data.txt");
    // ... use file ...
}  // Automatically closed

// Custom RAII class
class Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {
        std::cout << "Timer started" << std::endl;
    }
    
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Time: " << duration.count() << " ms" << std::endl;
    }
};

// Usage
void someFunction() {
    Timer timer;  // Starts timing
    // ... code ...
}  // Automatically prints time on destruction
```

---

## Quick Optimization Checklist

### ✅ Compiler Flags
- [ ] Use `-O2` or `-O3` for release builds
- [ ] Add `-march=native` for CPU-specific optimizations
- [ ] Consider `-flto` for link-time optimization

### ✅ Memory
- [ ] Use stack allocation for small, fixed-size data
- [ ] Reserve space in vectors to avoid reallocations
- [ ] Use contiguous memory (flat arrays) instead of nested containers
- [ ] Consider memory pools for frequent allocations

### ✅ Algorithms
- [ ] Choose appropriate data structures (vector, set, map, etc.)
- [ ] Use STL algorithms instead of manual loops
- [ ] Avoid unnecessary copies (use const references)
- [ ] Cache loop invariants

### ✅ Cache Optimization
- [ ] Access memory sequentially (row-major order)
- [ ] Keep frequently used data close together
- [ ] Use data structures that fit in cache

### ✅ Profiling
- [ ] Profile before optimizing
- [ ] Focus on hot paths (95% of time in 5% of code)
- [ ] Measure after each optimization

### ✅ Parallelization
- [ ] Identify independent computations
- [ ] Use threads/OpenMP for CPU-bound tasks
- [ ] Ensure thread safety with mutexes/atomics

---

## Performance Testing Example
```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

void runPerformanceTest() {
    const int size = 1000000;
    std::vector<int> data(size);
    
    // Initialize
    for (int i = 0; i < size; ++i) {
        data[i] = size - i;
    }
    
    // Test 1: STL sort
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(data.begin(), data.end());
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Sort time: " << duration.count() << " ms" << std::endl;
    
    // Test 2: Binary search
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        bool found = std::binary_search(data.begin(), data.end(), i);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "10000 binary searches: " << duration.count() << " ms" << std::endl;
}

int main() {
    std::cout << "Running performance tests..." << std::endl;
    runPerformanceTest();
    return 0;
}
```

**Your C++ environment is now optimized for high-performance computing!**
