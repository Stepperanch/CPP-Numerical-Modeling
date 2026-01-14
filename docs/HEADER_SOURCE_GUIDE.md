# Header/Source File Split Guide

## Directory Structure

```
CPP_Workspace/
├── include/              # Header files (.h)
│   └── vector3d.h
├── src/                  # Implementation files (.cpp)
│   └── vector3d.cpp
├── main.cpp              # Entry point (single file programs)
└── example_multi_file.cpp  # Example using the split structure
```

---

## Why Split Header and Source?

### Benefits:
1. **Separation of Interface and Implementation**
   - Header = "what it does" (public interface)
   - Source = "how it does it" (implementation)

2. **Faster Compilation**
   - Only recompile changed `.cpp` files
   - Headers included once, compiled separately

3. **Better Organization**
   - Clear project structure
   - Easy to find declarations vs implementations

4. **Code Reuse**
   - Include header in multiple files
   - Link implementation once

5. **Library Creation**
   - Distribute headers for API
   - Keep implementation private

---

## Header File Pattern (.h)

### Example: `include/vector3d.h`

```cpp
#ifndef VECTOR3D_H    // Include guard (prevents multiple inclusion)
#define VECTOR3D_H

#include <iostream>   // Dependencies

/**
 * @brief Class description
 */
class Vector3D {
private:
    double x, y, z;   // Member variables

public:
    // Constructors
    Vector3D();
    Vector3D(double x, double y, double z);
    
    // Methods (declaration only)
    double magnitude() const;
    Vector3D normalized() const;
    
    // Simple methods can be defined inline
    double getX() const { return x; }
};

#endif  // VECTOR3D_H
```

### Key Elements:
- **Include guards** (`#ifndef`, `#define`, `#endif`)
- **Declarations only** (no implementation)
- **Inline functions** (small getters/setters)
- **Documentation** (Doxygen comments)

---

## Source File Pattern (.cpp)

### Example: `src/vector3d.cpp`

```cpp
#include "../include/vector3d.h"
#include <cmath>  // Additional dependencies

// Constructor implementations
Vector3D::Vector3D() : x(0), y(0), z(0) {}

Vector3D::Vector3D(double x, double y, double z) 
    : x(x), y(y), z(z) {}

// Method implementations
double Vector3D::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3D Vector3D::normalized() const {
    double mag = magnitude();
    if (mag == 0) return Vector3D(0, 0, 0);
    return Vector3D(x/mag, y/mag, z/mag);
}
```

### Key Elements:
- **Include corresponding header first**
- **Implementations** of declared methods
- **Class scope** (`ClassName::methodName`)
- **No declarations** (only definitions)

---

## Using the Split Files

### In Your Main Program:

```cpp
// main.cpp
#include "include/vector3d.h"

int main() {
    Vector3D v(3, 4, 0);
    std::cout << "Magnitude: " << v.magnitude() << std::endl;
    return 0;
}
```

### Compilation:

**Method 1: Direct Compilation**
```bash
clang++ -std=c++17 main.cpp src/vector3d.cpp -o bin/program
```

**Method 2: Using Makefile** (recommended)
```bash
# Edit Makefile, update SOURCES:
SOURCES = main.cpp src/vector3d.cpp

# Then build:
make
```

**Method 3: Separate Compilation (fastest for large projects)**
```bash
# Compile to object files
clang++ -std=c++17 -c src/vector3d.cpp -o obj/vector3d.o
clang++ -std=c++17 -c main.cpp -o obj/main.o

# Link object files
clang++ obj/main.o obj/vector3d.o -o bin/program
```

---

## Testing the Example

### Run the provided example:
```bash
cd ~/Documents/CPP_Workspace

# Compile
clang++ -std=c++17 example_multi_file.cpp src/vector3d.cpp -o bin/example

# Run
./bin/example
```

### Expected output:
```
======================================
  Vector3D Class Example
======================================

Vector 1: (3, 4, 0)
Vector 2: (1, 0, 0)

Magnitude of v1: 5
Normalized v1: (0.6, 0.8, 0) (magnitude: 1)

v1 + v2 = (4, 4, 0)
v1 - v2 = (2, 4, 0)
v1 * 2 = (6, 8, 0)

v1 · v2 = 3

v2 × v3 = (0, 0, 1)

✓ Header/Source split working!
```

---

## Creating Your Own Classes

### Step 1: Create Header (`include/myclass.h`)
```cpp
#ifndef MYCLASS_H
#define MYCLASS_H

class MyClass {
private:
    int data;
    
public:
    MyClass(int data);
    void doSomething();
    int getData() const { return data; }
};

#endif
```

### Step 2: Create Source (`src/myclass.cpp`)
```cpp
#include "../include/myclass.h"

MyClass::MyClass(int data) : data(data) {}

void MyClass::doSomething() {
    // Implementation here
}
```

### Step 3: Use in Main
```cpp
#include "include/myclass.h"

int main() {
    MyClass obj(42);
    obj.doSomething();
    return 0;
}
```

### Step 4: Compile
```bash
clang++ -std=c++17 main.cpp src/myclass.cpp -o bin/program
```

---

## Best Practices

### ✅ Do:
- Use include guards in all headers
- Keep headers minimal (declarations only)
- Document public interfaces in headers
- Include headers you need, nothing more
- Use relative paths: `#include "../include/file.h"`

### ❌ Don't:
- Put implementations in headers (except inline)
- Include `.cpp` files
- Use `using namespace` in headers
- Forget include guards
- Have circular dependencies

---

## Common Patterns

### Multiple Classes
```
include/
├── vector3d.h
├── matrix.h
├── particle.h
└── integrator.h

src/
├── vector3d.cpp
├── matrix.cpp
├── particle.cpp
└── integrator.cpp
```

### Class Using Another Class
```cpp
// particle.h
#include "vector3d.h"  // Include dependency

class Particle {
    Vector3D position;  // Use Vector3D
    Vector3D velocity;
};
```

### Template Classes (special case)
```cpp
// Templates MUST be in headers (no .cpp file)
// template_class.h
template <typename T>
class MyTemplate {
    T data;
public:
    MyTemplate(T d) : data(d) {}  // Implementation in header
    T get() const { return data; }
};
```

---

## When to Use This Structure

### Use Header/Source Split When:
- ✅ Project has 3+ classes
- ✅ Classes are complex (50+ lines)
- ✅ Classes will be reused in multiple files
- ✅ Building a library
- ✅ Working in a team

### Stay with Single Files When:
- ❌ Quick scripts or small programs
- ❌ Learning/experimenting
- ❌ Less than 200 lines total
- ❌ Only one or two simple classes

---

## Makefile Integration

Update your `Makefile`:
```makefile
SOURCES = main.cpp \
          src/vector3d.cpp \
          src/matrix.cpp \
          src/particle.cpp

INCLUDES = -Iinclude

%.o: %.cpp
    $(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
```

Then just run `make`!

---

**Your workspace now has a complete header/source split example ready to use!**
