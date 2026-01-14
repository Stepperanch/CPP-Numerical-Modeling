# C++ Basics Reference

## 📚 Table of Contents
1. [Program Structure](#program-structure)
2. [Data Types](#data-types)
3. [Variables & Constants](#variables--constants)
4. [Operators](#operators)
5. [Control Flow](#control-flow)
6. [Functions](#functions)
7. [Arrays & Vectors](#arrays--vectors)
8. [Pointers & References](#pointers--references)
9. [String Handling](#string-handling)
10. [Input/Output](#inputoutput)

---

## Program Structure

### Minimal C++ Program
```cpp
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
```

### Components Explained
```cpp
#include <iostream>        // Preprocessor directive (import library)

int main() {              // Main function (entry point)
    // Your code here
    return 0;             // Return success code to OS
}
```

### Common Headers
```cpp
#include <iostream>       // Input/output streams (cout, cin)
#include <string>         // String class
#include <vector>         // Dynamic arrays
#include <cmath>          // Math functions (sin, cos, sqrt, etc.)
#include <fstream>        // File input/output
#include <algorithm>      // Algorithms (sort, find, etc.)
#include <iomanip>        // Output formatting
#include <chrono>         // Time and date utilities
```

### Namespaces
```cpp
// Method 1: Use std:: prefix (recommended)
std::cout << "Hello" << std::endl;

// Method 2: Using declaration (for specific items)
using std::cout;
using std::endl;
cout << "Hello" << endl;

// Method 3: Using directive (use sparingly)
using namespace std;
cout << "Hello" << endl;
```

---

## Data Types

### Fundamental Types
```cpp
// Integer types
int x = 42;                    // 4 bytes, -2^31 to 2^31-1
short y = 100;                 // 2 bytes
long z = 1000000L;             // 4 or 8 bytes
long long w = 9223372036854775807LL;  // 8 bytes
unsigned int u = 42;           // Only positive values

// Floating-point types
float pi = 3.14f;              // 4 bytes, ~6-7 decimal digits
double e = 2.718281828;        // 8 bytes, ~15 decimal digits
long double ld = 3.14159265358979L;  // 10+ bytes

// Character type
char c = 'A';                  // 1 byte, single character
char newline = '\n';           // Escape character

// Boolean type
bool isTrue = true;            // 1 byte, true or false
bool isFalse = false;

// Size type (for array indices, sizes)
size_t count = 100;            // Unsigned integer type
```

### Type Properties
```cpp
#include <iostream>
#include <limits>

int main() {
    std::cout << "int size: " << sizeof(int) << " bytes" << std::endl;
    std::cout << "int max: " << std::numeric_limits<int>::max() << std::endl;
    std::cout << "double precision: " << std::numeric_limits<double>::digits10 << " digits" << std::endl;
    return 0;
}
```

---

## Variables & Constants

### Variable Declaration
```cpp
// Declaration
int x;              // Declare variable

// Initialization
int y = 10;         // Declare and initialize
int z(20);          // Alternative initialization
int w{30};          // Uniform initialization (C++11)

// Multiple declarations
int a = 1, b = 2, c = 3;

// Auto type deduction (C++11)
auto value = 42;           // Compiler deduces 'int'
auto pi = 3.14;            // Compiler deduces 'double'
```

### Constants
```cpp
// Compile-time constant (preferred)
const double PI = 3.14159265359;

// Constant expression (C++11)
constexpr int MAX_SIZE = 100;
constexpr double SPEED_OF_LIGHT = 299792458.0;  // m/s

// Preprocessor macro (old style, avoid when possible)
#define GRAVITY 9.8
```

### Type Casting
```cpp
// C-style cast (avoid)
int x = (int)3.14;

// C++ static cast (preferred)
double pi = 3.14159;
int approx = static_cast<int>(pi);  // Result: 3

// Implicit conversion
double d = 3.14;
int i = d;  // Implicit cast, i = 3
```

---

## Operators

### Arithmetic Operators
```cpp
int a = 10, b = 3;

int sum = a + b;         // Addition: 13
int diff = a - b;        // Subtraction: 7
int prod = a * b;        // Multiplication: 30
int quot = a / b;        // Integer division: 3
int rem = a % b;         // Modulus (remainder): 1

double x = 10.0, y = 3.0;
double div = x / y;      // Floating-point division: 3.333...
```

### Increment/Decrement
```cpp
int x = 5;

x++;        // Post-increment: use value, then increment
++x;        // Pre-increment: increment, then use value
x--;        // Post-decrement
--x;        // Pre-decrement

// Example difference
int a = 5;
int b = a++;    // b = 5, a = 6
int c = ++a;    // c = 7, a = 7
```

### Comparison Operators
```cpp
int a = 5, b = 10;

bool equal = (a == b);       // Equal to: false
bool notEqual = (a != b);    // Not equal to: true
bool less = (a < b);         // Less than: true
bool greater = (a > b);      // Greater than: false
bool lessEq = (a <= b);      // Less than or equal: true
bool greaterEq = (a >= b);   // Greater than or equal: false
```

### Logical Operators
```cpp
bool a = true, b = false;

bool andResult = a && b;     // Logical AND: false
bool orResult = a || b;      // Logical OR: true
bool notResult = !a;         // Logical NOT: false

// Short-circuit evaluation
if (x != 0 && y / x > 2) {   // Second part only evaluated if x != 0
    // Safe division
}
```

### Bitwise Operators
```cpp
unsigned int a = 60;    // Binary: 0011 1100
unsigned int b = 13;    // Binary: 0000 1101

unsigned int andOp = a & b;   // AND: 0000 1100 (12)
unsigned int orOp = a | b;    // OR:  0011 1101 (61)
unsigned int xorOp = a ^ b;   // XOR: 0011 0001 (49)
unsigned int notOp = ~a;      // NOT: 1100 0011

unsigned int leftShift = a << 2;   // Left shift: 1111 0000 (240)
unsigned int rightShift = a >> 2;  // Right shift: 0000 1111 (15)
```

### Assignment Operators
```cpp
int x = 10;

x += 5;     // x = x + 5  (15)
x -= 3;     // x = x - 3  (12)
x *= 2;     // x = x * 2  (24)
x /= 4;     // x = x / 4  (6)
x %= 4;     // x = x % 4  (2)
x &= 3;     // x = x & 3
x |= 1;     // x = x | 1
x ^= 1;     // x = x ^ 1
x <<= 1;    // x = x << 1
x >>= 1;    // x = x >> 1
```

---

## Control Flow

### If-Else Statements
```cpp
int x = 10;

// Simple if
if (x > 5) {
    std::cout << "x is greater than 5" << std::endl;
}

// If-else
if (x > 10) {
    std::cout << "Greater than 10" << std::endl;
} else {
    std::cout << "10 or less" << std::endl;
}

// If-else if-else
if (x < 0) {
    std::cout << "Negative" << std::endl;
} else if (x == 0) {
    std::cout << "Zero" << std::endl;
} else {
    std::cout << "Positive" << std::endl;
}

// Ternary operator
int max = (a > b) ? a : b;
std::cout << "Max: " << max << std::endl;
```

### Switch Statement
```cpp
int day = 3;

switch (day) {
    case 1:
        std::cout << "Monday" << std::endl;
        break;
    case 2:
        std::cout << "Tuesday" << std::endl;
        break;
    case 3:
        std::cout << "Wednesday" << std::endl;
        break;
    default:
        std::cout << "Other day" << std::endl;
        break;
}
```

### For Loop
```cpp
// Traditional for loop
for (int i = 0; i < 10; ++i) {
    std::cout << i << " ";
}

// Range-based for loop (C++11)
std::vector<int> numbers = {1, 2, 3, 4, 5};
for (int num : numbers) {
    std::cout << num << " ";
}

// With reference (modify elements)
for (int& num : numbers) {
    num *= 2;
}

// Const reference (read-only, efficient)
for (const auto& num : numbers) {
    std::cout << num << " ";
}
```

### While Loop
```cpp
int i = 0;
while (i < 10) {
    std::cout << i << " ";
    ++i;
}

// Infinite loop (break to exit)
while (true) {
    std::cout << "Enter 'q' to quit: ";
    char c;
    std::cin >> c;
    if (c == 'q') break;
}
```

### Do-While Loop
```cpp
int i = 0;
do {
    std::cout << i << " ";
    ++i;
} while (i < 10);

// Executes at least once
int x = 20;
do {
    std::cout << "This runs once even though x >= 10" << std::endl;
} while (x < 10);
```

### Break and Continue
```cpp
// Break: exit loop immediately
for (int i = 0; i < 10; ++i) {
    if (i == 5) break;
    std::cout << i << " ";  // Output: 0 1 2 3 4
}

// Continue: skip to next iteration
for (int i = 0; i < 10; ++i) {
    if (i % 2 == 0) continue;
    std::cout << i << " ";  // Output: 1 3 5 7 9
}
```

---

## Functions

### Function Definition
```cpp
// Return type, name, parameters
int add(int a, int b) {
    return a + b;
}

// Void function (no return value)
void printMessage() {
    std::cout << "Hello!" << std::endl;
}

// Function with default parameters
double power(double base, int exponent = 2) {
    double result = 1.0;
    for (int i = 0; i < exponent; ++i) {
        result *= base;
    }
    return result;
}

// Usage
int sum = add(5, 3);              // 8
double square = power(5.0);        // 25.0 (uses default exponent)
double cube = power(5.0, 3);       // 125.0
```

### Function Overloading
```cpp
// Same name, different parameters
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}

int add(int a, int b, int c) {
    return a + b + c;
}

// Compiler selects based on arguments
int x = add(5, 3);           // Calls int version
double y = add(5.0, 3.0);    // Calls double version
int z = add(1, 2, 3);        // Calls three-parameter version
```

### Pass by Value vs Reference
```cpp
// Pass by value (copy)
void modifyValue(int x) {
    x = 100;  // Only modifies local copy
}

// Pass by reference (modify original)
void modifyReference(int& x) {
    x = 100;  // Modifies original variable
}

// Pass by const reference (read-only, efficient)
void printVector(const std::vector<int>& vec) {
    for (int num : vec) {
        std::cout << num << " ";
    }
}

int a = 10;
modifyValue(a);      // a is still 10
modifyReference(a);  // a is now 100
```

### Inline Functions
```cpp
// Hint to compiler to insert function code directly
inline int square(int x) {
    return x * x;
}

// Useful for small, frequently called functions
```

### Lambda Functions (C++11)
```cpp
// Syntax: [capture](parameters) -> return_type { body }

// Simple lambda
auto add = [](int a, int b) { return a + b; };
int result = add(5, 3);  // 8

// Capture external variables
int multiplier = 10;
auto multiply = [multiplier](int x) { return x * multiplier; };
int result2 = multiply(5);  // 50

// Capture by reference
int counter = 0;
auto increment = [&counter]() { counter++; };
increment();
increment();  // counter is now 2

// Use with algorithms
std::vector<int> numbers = {1, 2, 3, 4, 5};
std::for_each(numbers.begin(), numbers.end(), [](int n) {
    std::cout << n * n << " ";  // Print squares
});
```

---

## Arrays & Vectors

### C-Style Arrays
```cpp
// Fixed-size array
int numbers[5] = {1, 2, 3, 4, 5};

// Access elements
int first = numbers[0];      // 1
int last = numbers[4];       // 5

// Partial initialization
int values[10] = {1, 2, 3};  // Rest initialized to 0

// Size calculation
int size = sizeof(numbers) / sizeof(numbers[0]);  // 5

// Iterate
for (int i = 0; i < 5; ++i) {
    std::cout << numbers[i] << " ";
}

// Multidimensional arrays
int matrix[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};
int element = matrix[1][2];  // 6
```

### Vectors (Dynamic Arrays)
```cpp
#include <vector>

// Declaration
std::vector<int> numbers;

// Initialize with values
std::vector<int> values = {1, 2, 3, 4, 5};

// Initialize with size
std::vector<double> data(100);        // 100 elements, default value 0.0
std::vector<double> data2(100, 3.14); // 100 elements, all 3.14

// Add elements
values.push_back(6);       // Add to end
values.push_back(7);

// Access elements
int first = values[0];           // No bounds checking
int second = values.at(1);       // With bounds checking

// Size and capacity
size_t count = values.size();    // Number of elements
size_t space = values.capacity(); // Allocated space

// Remove elements
values.pop_back();          // Remove last element
values.clear();             // Remove all elements

// Iterate
for (size_t i = 0; i < values.size(); ++i) {
    std::cout << values[i] << " ";
}

// Range-based for
for (int num : values) {
    std::cout << num << " ";
}

// Check if empty
if (values.empty()) {
    std::cout << "Vector is empty" << std::endl;
}

// Resize
values.resize(10);          // Change size (may add or remove elements)

// 2D vector
std::vector<std::vector<int>> matrix(3, std::vector<int>(3, 0));
matrix[0][0] = 1;
```

---

## Pointers & References

### Pointers
```cpp
int x = 42;
int* ptr = &x;        // Pointer to x (stores address)

// Dereference: access value at address
int value = *ptr;     // value = 42

// Modify through pointer
*ptr = 100;           // x is now 100

// Null pointer
int* nullPtr = nullptr;  // C++11 (preferred over NULL or 0)

// Pointer arithmetic
int arr[5] = {1, 2, 3, 4, 5};
int* p = arr;         // Points to first element
p++;                  // Points to second element
int val = *p;         // val = 2

// Dynamic memory allocation
int* dynamicInt = new int;        // Allocate single int
*dynamicInt = 50;
delete dynamicInt;                // Free memory

int* dynamicArray = new int[100]; // Allocate array
delete[] dynamicArray;            // Free array
```

### References
```cpp
int x = 42;
int& ref = x;         // Reference to x (alias)

// Modify through reference
ref = 100;            // x is now 100

// Reference must be initialized
int& invalid;         // ERROR: must initialize

// Reference cannot be reassigned
int y = 50;
ref = y;              // This changes x's value to 50, doesn't make ref refer to y

// Const reference
const int& constRef = x;
// constRef = 200;    // ERROR: cannot modify through const reference
```

### Pointer vs Reference
```cpp
// Pointer: can be null, can be reassigned, requires dereferencing
int* ptr = nullptr;
if (ptr != nullptr) {
    *ptr = 100;
}

// Reference: cannot be null, cannot be reassigned, no dereferencing needed
int x = 42;
int& ref = x;
ref = 100;  // Direct access, no * needed
```

---

## String Handling

### C++ Strings
```cpp
#include <string>

// Declaration and initialization
std::string str1 = "Hello";
std::string str2("World");
std::string str3 = str1;  // Copy

// Concatenation
std::string greeting = str1 + " " + str2;  // "Hello World"
str1 += " there";                          // "Hello there"

// Access characters
char first = str1[0];          // 'H'
char second = str1.at(1);      // 'e' (with bounds checking)

// Length
size_t len = str1.length();    // or str1.size()

// Substring
std::string sub = greeting.substr(0, 5);  // "Hello"

// Find
size_t pos = greeting.find("World");      // Position of "World"
if (pos != std::string::npos) {
    std::cout << "Found at position " << pos << std::endl;
}

// Replace
greeting.replace(0, 5, "Hi");             // "Hi World"

// Compare
if (str1 == str2) {
    std::cout << "Equal" << std::endl;
}

int cmp = str1.compare(str2);  // <0, 0, or >0

// Convert to C-string
const char* cstr = greeting.c_str();

// Convert numbers to string
int num = 42;
std::string numStr = std::to_string(num);

// Convert string to number
std::string s = "123";
int value = std::stoi(s);           // String to int
double d = std::stod("3.14");       // String to double
long l = std::stol("1000000");      // String to long

// Iterate through characters
for (char c : greeting) {
    std::cout << c << " ";
}

// Modify characters
for (char& c : greeting) {
    c = std::toupper(c);  // Convert to uppercase
}

// Clear string
greeting.clear();
if (greeting.empty()) {
    std::cout << "String is empty" << std::endl;
}
```

### String Streams
```cpp
#include <sstream>

// Convert types to string
std::ostringstream oss;
oss << "Value: " << 42 << ", Pi: " << 3.14;
std::string result = oss.str();

// Parse string
std::string data = "10 20 30";
std::istringstream iss(data);
int a, b, c;
iss >> a >> b >> c;  // a=10, b=20, c=30
```

---

## Input/Output

### Console Output
```cpp
#include <iostream>

// Basic output
std::cout << "Hello, World!" << std::endl;
std::cout << "Value: " << 42 << std::endl;

// Multiple items
int x = 10;
double y = 3.14;
std::cout << "x = " << x << ", y = " << y << std::endl;

// No newline
std::cout << "Text " << std::flush;  // Flush buffer without newline
```

### Console Input
```cpp
#include <iostream>
#include <string>

// Read integer
int num;
std::cout << "Enter a number: ";
std::cin >> num;

// Read multiple values
int a, b;
std::cout << "Enter two numbers: ";
std::cin >> a >> b;

// Read string (single word)
std::string word;
std::cout << "Enter a word: ";
std::cin >> word;

// Read entire line
std::string line;
std::cout << "Enter a sentence: ";
std::cin.ignore();  // Clear buffer
std::getline(std::cin, line);

// Clear error state
std::cin.clear();
std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
```

### Formatted Output
```cpp
#include <iostream>
#include <iomanip>

double pi = 3.14159265359;

// Set precision
std::cout << std::fixed << std::setprecision(2);
std::cout << pi << std::endl;  // 3.14

std::cout << std::setprecision(6);
std::cout << pi << std::endl;  // 3.141593

// Field width
std::cout << std::setw(10) << "Name" << std::setw(10) << "Age" << std::endl;
std::cout << std::setw(10) << "Alice" << std::setw(10) << 30 << std::endl;

// Alignment
std::cout << std::left << std::setw(10) << "Left" << std::endl;
std::cout << std::right << std::setw(10) << "Right" << std::endl;

// Fill character
std::cout << std::setfill('*') << std::setw(10) << 42 << std::endl;  // ********42

// Boolean as text
std::cout << std::boolalpha;
std::cout << (5 > 3) << std::endl;  // "true" instead of "1"
```

### File I/O
```cpp
#include <fstream>
#include <string>

// Write to file
std::ofstream outFile("output.txt");
if (outFile.is_open()) {
    outFile << "Hello, File!" << std::endl;
    outFile << "Line 2" << std::endl;
    outFile.close();
}

// Read from file (line by line)
std::ifstream inFile("input.txt");
if (inFile.is_open()) {
    std::string line;
    while (std::getline(inFile, line)) {
        std::cout << line << std::endl;
    }
    inFile.close();
}

// Read word by word
std::ifstream inFile2("data.txt");
std::string word;
while (inFile2 >> word) {
    std::cout << word << std::endl;
}
inFile2.close();

// Append to file
std::ofstream appendFile("log.txt", std::ios::app);
appendFile << "New log entry" << std::endl;
appendFile.close();

// Check if file exists
std::ifstream testFile("test.txt");
if (testFile.good()) {
    std::cout << "File exists" << std::endl;
} else {
    std::cout << "File does not exist" << std::endl;
}
testFile.close();
```

---

## Quick Reference Card

### Common Patterns
```cpp
// Swap two variables
int a = 5, b = 10;
int temp = a;
a = b;
b = temp;

// Or use std::swap
std::swap(a, b);

// Find min/max
int min = std::min(a, b);
int max = std::max(a, b);

// Absolute value
int abs_val = std::abs(-42);  // 42
double abs_d = std::fabs(-3.14);  // 3.14

// Random numbers
#include <random>
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 100);
int random_num = dis(gen);  // Random int from 1 to 100

// Sort array/vector
std::sort(vec.begin(), vec.end());  // Ascending
std::sort(vec.begin(), vec.end(), std::greater<int>());  // Descending
```

**Continue to FUNCTIONS_REFERENCE.md for advanced topics!**
