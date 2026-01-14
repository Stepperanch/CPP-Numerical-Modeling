# C++ Functions & Advanced Features Reference

## 📚 Table of Contents
1. [Classes & Objects](#classes--objects)
2. [Constructors & Destructors](#constructors--destructors)
3. [Templates](#templates)
4. [Standard Library Containers](#standard-library-containers)
5. [Algorithms](#algorithms)
6. [Smart Pointers](#smart-pointers)
7. [Exception Handling](#exception-handling)
8. [File Handling Advanced](#file-handling-advanced)

---

## Classes & Objects

### Basic Class Definition
```cpp
class Rectangle {
private:
    double width;
    double height;

public:
    // Constructor
    Rectangle(double w, double h) : width(w), height(h) {}
    
    // Member functions
    double area() const {
        return width * height;
    }
    
    double perimeter() const {
        return 2 * (width + height);
    }
    
    // Getters
    double getWidth() const { return width; }
    double getHeight() const { return height; }
    
    // Setters
    void setWidth(double w) { width = w; }
    void setHeight(double h) { height = h; }
};

// Usage
Rectangle rect(5.0, 3.0);
std::cout << "Area: " << rect.area() << std::endl;
```

### Access Specifiers
```cpp
class Example {
private:
    int privateVar;      // Only accessible within class
    
protected:
    int protectedVar;    // Accessible in class and derived classes
    
public:
    int publicVar;       // Accessible everywhere
    
    void publicMethod() {
        privateVar = 10;    // OK
        protectedVar = 20;  // OK
    }
};
```

### Inheritance
```cpp
// Base class
class Shape {
protected:
    std::string color;
    
public:
    Shape(const std::string& c) : color(c) {}
    
    virtual double area() const = 0;  // Pure virtual (abstract)
    
    virtual void draw() const {
        std::cout << "Drawing a " << color << " shape" << std::endl;
    }
    
    virtual ~Shape() {}  // Virtual destructor
};

// Derived class
class Circle : public Shape {
private:
    double radius;
    
public:
    Circle(const std::string& c, double r) : Shape(c), radius(r) {}
    
    double area() const override {
        return 3.14159 * radius * radius;
    }
    
    void draw() const override {
        std::cout << "Drawing a " << color << " circle" << std::endl;
    }
};

// Usage
Circle circle("red", 5.0);
circle.draw();
std::cout << "Area: " << circle.area() << std::endl;

// Polymorphism
Shape* shape = new Circle("blue", 3.0);
std::cout << shape->area() << std::endl;
delete shape;
```

### Encapsulation Example
```cpp
class BankAccount {
private:
    std::string accountNumber;
    double balance;
    
    bool isValidAmount(double amount) const {
        return amount > 0;
    }
    
public:
    BankAccount(const std::string& accNum, double initialBalance)
        : accountNumber(accNum), balance(initialBalance) {}
    
    bool deposit(double amount) {
        if (isValidAmount(amount)) {
            balance += amount;
            return true;
        }
        return false;
    }
    
    bool withdraw(double amount) {
        if (isValidAmount(amount) && amount <= balance) {
            balance -= amount;
            return true;
        }
        return false;
    }
    
    double getBalance() const { return balance; }
    std::string getAccountNumber() const { return accountNumber; }
};
```

---

## Constructors & Destructors

### Constructor Types
```cpp
class Vector3D {
private:
    double x, y, z;
    
public:
    // Default constructor
    Vector3D() : x(0), y(0), z(0) {
        std::cout << "Default constructor" << std::endl;
    }
    
    // Parameterized constructor
    Vector3D(double x, double y, double z) : x(x), y(y), z(z) {
        std::cout << "Parameterized constructor" << std::endl;
    }
    
    // Copy constructor
    Vector3D(const Vector3D& other) : x(other.x), y(other.y), z(other.z) {
        std::cout << "Copy constructor" << std::endl;
    }
    
    // Move constructor (C++11)
    Vector3D(Vector3D&& other) noexcept
        : x(other.x), y(other.y), z(other.z) {
        std::cout << "Move constructor" << std::endl;
        other.x = other.y = other.z = 0;
    }
    
    // Copy assignment operator
    Vector3D& operator=(const Vector3D& other) {
        std::cout << "Copy assignment" << std::endl;
        if (this != &other) {
            x = other.x;
            y = other.y;
            z = other.z;
        }
        return *this;
    }
    
    // Move assignment operator (C++11)
    Vector3D& operator=(Vector3D&& other) noexcept {
        std::cout << "Move assignment" << std::endl;
        if (this != &other) {
            x = other.x;
            y = other.y;
            z = other.z;
            other.x = other.y = other.z = 0;
        }
        return *this;
    }
    
    // Destructor
    ~Vector3D() {
        std::cout << "Destructor" << std::endl;
    }
    
    double magnitude() const {
        return std::sqrt(x*x + y*y + z*z);
    }
};

// Usage
Vector3D v1;                      // Default constructor
Vector3D v2(1, 2, 3);            // Parameterized constructor
Vector3D v3 = v2;                // Copy constructor
Vector3D v4(std::move(v2));      // Move constructor
v1 = v3;                         // Copy assignment
v1 = Vector3D(4, 5, 6);          // Move assignment
```

### Initialization Lists (Preferred)
```cpp
class Point {
private:
    const int id;  // Must be initialized in initializer list
    double x, y;
    
public:
    // Preferred: initializer list
    Point(int id, double x, double y) : id(id), x(x), y(y) {}
    
    // Not preferred: assignment in body
    // Point(int id, double x, double y) {
    //     this->id = id;  // ERROR: can't assign to const
    //     this->x = x;
    //     this->y = y;
    // }
};
```

---

## Templates

### Function Templates
```cpp
// Generic swap function
template <typename T>
void swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

// Generic max function
template <typename T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

// Multiple template parameters
template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

// Usage
int x = 5, y = 10;
swap(x, y);  // Compiler deduces T = int

double d1 = 3.14, d2 = 2.71;
double maxD = max(d1, d2);  // T = double

auto result = add(5, 3.14);  // T = int, U = double
```

### Class Templates
```cpp
template <typename T>
class Stack {
private:
    std::vector<T> elements;
    
public:
    void push(const T& element) {
        elements.push_back(element);
    }
    
    T pop() {
        if (elements.empty()) {
            throw std::out_of_range("Stack is empty");
        }
        T top = elements.back();
        elements.pop_back();
        return top;
    }
    
    bool isEmpty() const {
        return elements.empty();
    }
    
    size_t size() const {
        return elements.size();
    }
};

// Usage
Stack<int> intStack;
intStack.push(10);
intStack.push(20);
int value = intStack.pop();  // 20

Stack<std::string> stringStack;
stringStack.push("Hello");
stringStack.push("World");
```

### Template Specialization
```cpp
// Generic template
template <typename T>
class MyClass {
public:
    void print() {
        std::cout << "Generic version" << std::endl;
    }
};

// Specialization for int
template <>
class MyClass<int> {
public:
    void print() {
        std::cout << "Specialized for int" << std::endl;
    }
};

// Usage
MyClass<double> obj1;
obj1.print();  // "Generic version"

MyClass<int> obj2;
obj2.print();  // "Specialized for int"
```

---

## Standard Library Containers

### Vector
```cpp
#include <vector>

std::vector<int> vec = {1, 2, 3, 4, 5};

// Add elements
vec.push_back(6);
vec.insert(vec.begin(), 0);  // Insert at beginning

// Remove elements
vec.pop_back();
vec.erase(vec.begin());  // Remove first element

// Access
int first = vec.front();
int last = vec.back();
int third = vec[2];

// Size operations
vec.resize(10);
vec.reserve(100);  // Reserve capacity
vec.shrink_to_fit();  // Release unused memory

// Clear
vec.clear();
```

### Map (Key-Value Pairs)
```cpp
#include <map>

std::map<std::string, int> ages;

// Insert
ages["Alice"] = 30;
ages["Bob"] = 25;
ages.insert({"Charlie", 35});

// Access
int aliceAge = ages["Alice"];

// Check if key exists
if (ages.find("David") != ages.end()) {
    std::cout << "David's age: " << ages["David"] << std::endl;
} else {
    std::cout << "David not found" << std::endl;
}

// Iterate
for (const auto& pair : ages) {
    std::cout << pair.first << ": " << pair.second << std::endl;
}

// Remove
ages.erase("Bob");

// Size
std::cout << "Size: " << ages.size() << std::endl;
```

### Set (Unique Elements)
```cpp
#include <set>

std::set<int> numbers = {5, 2, 8, 1, 9};

// Insert
numbers.insert(3);
numbers.insert(5);  // Duplicate, not added

// Check if element exists
if (numbers.count(5) > 0) {
    std::cout << "5 is in the set" << std::endl;
}

// Iterate (sorted order)
for (int num : numbers) {
    std::cout << num << " ";  // 1 2 3 5 8 9
}

// Remove
numbers.erase(5);

// Find
auto it = numbers.find(8);
if (it != numbers.end()) {
    std::cout << "Found: " << *it << std::endl;
}
```

### Queue & Stack
```cpp
#include <queue>
#include <stack>

// Queue (FIFO)
std::queue<int> q;
q.push(1);
q.push(2);
q.push(3);
int front = q.front();  // 1
q.pop();  // Remove 1

// Stack (LIFO)
std::stack<int> s;
s.push(1);
s.push(2);
s.push(3);
int top = s.top();  // 3
s.pop();  // Remove 3
```

### Pair & Tuple
```cpp
#include <utility>
#include <tuple>

// Pair
std::pair<int, std::string> person = {25, "Alice"};
int age = person.first;
std::string name = person.second;

// Tuple
std::tuple<int, std::string, double> data = {42, "Answer", 3.14};
int value = std::get<0>(data);
std::string text = std::get<1>(data);
double pi = std::get<2>(data);

// Structured bindings (C++17)
auto [x, y, z] = data;
std::cout << x << ", " << y << ", " << z << std::endl;
```

---

## Algorithms

### Sorting
```cpp
#include <algorithm>
#include <vector>

std::vector<int> vec = {5, 2, 8, 1, 9};

// Sort ascending
std::sort(vec.begin(), vec.end());

// Sort descending
std::sort(vec.begin(), vec.end(), std::greater<int>());

// Sort with custom comparator
std::sort(vec.begin(), vec.end(), [](int a, int b) {
    return a > b;  // Descending
});

// Partial sort (first 3 elements)
std::partial_sort(vec.begin(), vec.begin() + 3, vec.end());

// Check if sorted
bool isSorted = std::is_sorted(vec.begin(), vec.end());
```

### Searching
```cpp
#include <algorithm>

std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9};

// Find element
auto it = std::find(vec.begin(), vec.end(), 5);
if (it != vec.end()) {
    std::cout << "Found at index: " << (it - vec.begin()) << std::endl;
}

// Binary search (vector must be sorted)
bool found = std::binary_search(vec.begin(), vec.end(), 5);

// Lower bound (first element >= value)
auto lb = std::lower_bound(vec.begin(), vec.end(), 5);

// Upper bound (first element > value)
auto ub = std::upper_bound(vec.begin(), vec.end(), 5);

// Count occurrences
int count = std::count(vec.begin(), vec.end(), 5);
```

### Transforming
```cpp
#include <algorithm>

std::vector<int> vec = {1, 2, 3, 4, 5};
std::vector<int> result(vec.size());

// Transform (square each element)
std::transform(vec.begin(), vec.end(), result.begin(),
               [](int x) { return x * x; });

// For each
std::for_each(vec.begin(), vec.end(), [](int x) {
    std::cout << x << " ";
});

// Accumulate (sum)
#include <numeric>
int sum = std::accumulate(vec.begin(), vec.end(), 0);

// Accumulate with operation
int product = std::accumulate(vec.begin(), vec.end(), 1,
                              [](int a, int b) { return a * b; });
```

### Min/Max
```cpp
#include <algorithm>

std::vector<int> vec = {5, 2, 8, 1, 9};

// Min/Max element
auto minIt = std::min_element(vec.begin(), vec.end());
auto maxIt = std::max_element(vec.begin(), vec.end());
std::cout << "Min: " << *minIt << ", Max: " << *maxIt << std::endl;

// Min/Max of two values
int a = 5, b = 10;
int minVal = std::min(a, b);
int maxVal = std::max(a, b);

// MinMax
auto [min, max] = std::minmax({5, 2, 8, 1, 9});
```

---

## Smart Pointers

### unique_ptr (Exclusive Ownership)
```cpp
#include <memory>

// Create unique_ptr
std::unique_ptr<int> ptr = std::make_unique<int>(42);

// Access
std::cout << *ptr << std::endl;

// Transfer ownership (move)
std::unique_ptr<int> ptr2 = std::move(ptr);
// ptr is now nullptr

// Array
std::unique_ptr<int[]> arr = std::make_unique<int[]>(10);
arr[0] = 1;

// Custom class
class MyClass {
public:
    MyClass() { std::cout << "Constructor" << std::endl; }
    ~MyClass() { std::cout << "Destructor" << std::endl; }
    void doSomething() { std::cout << "Doing something" << std::endl; }
};

std::unique_ptr<MyClass> obj = std::make_unique<MyClass>();
obj->doSomething();
// Automatically deleted when ptr goes out of scope
```

### shared_ptr (Shared Ownership)
```cpp
#include <memory>

// Create shared_ptr
std::shared_ptr<int> ptr1 = std::make_shared<int>(42);

// Share ownership
std::shared_ptr<int> ptr2 = ptr1;
std::shared_ptr<int> ptr3 = ptr1;

// Reference count
std::cout << "Use count: " << ptr1.use_count() << std::endl;  // 3

// Object deleted when last shared_ptr is destroyed
ptr2.reset();  // Reference count: 2
ptr3.reset();  // Reference count: 1
// ptr1 goes out of scope, object is deleted
```

### weak_ptr (Break Circular References)
```cpp
#include <memory>

class Node {
public:
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // Use weak_ptr to break cycle
    int data;
    
    Node(int val) : data(val) {}
    ~Node() { std::cout << "Node " << data << " destroyed" << std::endl; }
};

auto node1 = std::make_shared<Node>(1);
auto node2 = std::make_shared<Node>(2);

node1->next = node2;
node2->prev = node1;  // weak_ptr doesn't increase reference count

// Check if weak_ptr is valid
if (auto prev = node2->prev.lock()) {
    std::cout << "Previous node data: " << prev->data << std::endl;
}
```

---

## Exception Handling

### Basic Try-Catch
```cpp
#include <stdexcept>

try {
    int x = 10;
    int y = 0;
    
    if (y == 0) {
        throw std::runtime_error("Division by zero");
    }
    
    int result = x / y;
} catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "General exception: " << e.what() << std::endl;
} catch (...) {
    std::cerr << "Unknown exception" << std::endl;
}
```

### Standard Exception Classes
```cpp
#include <stdexcept>

// Logic errors (programming bugs)
throw std::logic_error("Logic error");
throw std::invalid_argument("Invalid argument");
throw std::out_of_range("Out of range");

// Runtime errors
throw std::runtime_error("Runtime error");
throw std::overflow_error("Overflow error");
throw std::underflow_error("Underflow error");
```

### Custom Exceptions
```cpp
class MyException : public std::exception {
private:
    std::string message;
    
public:
    MyException(const std::string& msg) : message(msg) {}
    
    const char* what() const noexcept override {
        return message.c_str();
    }
};

try {
    throw MyException("Something went wrong");
} catch (const MyException& e) {
    std::cerr << "Custom exception: " << e.what() << std::endl;
}
```

### RAII (Resource Acquisition Is Initialization)
```cpp
class FileHandler {
private:
    std::ofstream file;
    
public:
    FileHandler(const std::string& filename) {
        file.open(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file");
        }
    }
    
    ~FileHandler() {
        if (file.is_open()) {
            file.close();
        }
    }
    
    void write(const std::string& text) {
        file << text << std::endl;
    }
};

try {
    FileHandler handler("data.txt");
    handler.write("Hello");
    // File automatically closed when handler goes out of scope
} catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
}
```

---

## File Handling Advanced

### Binary Files
```cpp
#include <fstream>

// Write binary
std::ofstream outFile("data.bin", std::ios::binary);
int numbers[] = {1, 2, 3, 4, 5};
outFile.write(reinterpret_cast<char*>(numbers), sizeof(numbers));
outFile.close();

// Read binary
std::ifstream inFile("data.bin", std::ios::binary);
int readNumbers[5];
inFile.read(reinterpret_cast<char*>(readNumbers), sizeof(readNumbers));
inFile.close();
```

### File Positioning
```cpp
#include <fstream>

std::fstream file("data.txt", std::ios::in | std::ios::out);

// Move to beginning
file.seekg(0, std::ios::beg);

// Move to end
file.seekg(0, std::ios::end);

// Get current position
std::streampos pos = file.tellg();

// Move to specific position
file.seekg(pos);

file.close();
```

### CSV Reading
```cpp
#include <fstream>
#include <sstream>
#include <vector>

std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    std::string line;
    
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        
        data.push_back(row);
    }
    
    file.close();
    return data;
}

// Usage
auto data = readCSV("data.csv");
for (const auto& row : data) {
    for (const auto& cell : row) {
        std::cout << cell << " ";
    }
    std::cout << std::endl;
}
```

**Continue to NUMERICAL_PHYSICS.md for physics-specific techniques!**
