# Advanced Features - Quick Reference

This guide explains how to use the advanced features added to your C++ workspace.

---

## 1️⃣ Code Formatting (ClangFormat)

### Auto-format on Save
Already enabled! Your code formats automatically when you save.

### Manual Formatting
- **Format entire file**: Right-click → "Format Document" or **Shift+Option+F**
- **Format selection**: Select code → Right-click → "Format Selection"

### Example:
```cpp
// Before
int main(){int x=5;if(x>3){cout<<"yes";}}

// After (automatic)
int main() {
    int x = 5;
    if (x > 3) {
        cout << "yes";
    }
}
```

---

## 2️⃣ Multiple Build Configurations

### Access Build Tasks
Press **Cmd+Shift+P** → Type "Run Task" → Select:

**Standard Builds:**
- **Build C++** - Default (O2 optimization, debug symbols)
- **Debug Build** - No optimization, full debugging (O0)
- **Release Build** - Maximum optimization (O3)
- **Profile Build** - With profiling enabled

**Sanitizer Builds:**
- **Build with Address Sanitizer** - Detects memory errors
- **Build with Undefined Behavior Sanitizer** - Finds undefined behavior

### When to Use Each:

| Build Type | Use When |
|------------|----------|
| **Standard** | Daily development |
| **Debug** | Using breakpoints, step-through debugging |
| **Release** | Final performance, benchmarking |
| **Profile** | Finding performance bottlenecks |
| **AddressSanitizer** | Hunting memory bugs |
| **UBSanitizer** | Checking for undefined behavior |

### Output Files:
- Standard: `bin/program`
- Debug: `bin/program_debug`
- Release: `bin/program_release`
- Profile: `bin/program_profile`
- ASan: `bin/program_asan`
- UBSan: `bin/program_ubsan`

---

## 3️⃣ Makefile

### Basic Usage:
```bash
cd ~/Documents/CPP_Workspace

make          # Build program
make run      # Build and run
make clean    # Remove binaries
make debug    # Debug build
make release  # Release build
```

### When You Have Multiple Files:

**Edit Makefile** and update the SOURCES line:
```makefile
SOURCES = main.cpp src/vector3d.cpp src/particle.cpp
```

Then `make` only recompiles changed files!

---

## 4️⃣ Code Snippets

### Type abbreviation + Tab:

**Basic:**
- `main` + Tab → main function
- `fori` + Tab → for loop with index
- `forr` + Tab → range-based for loop
- `vec` + Tab → vector declaration
- `class` + Tab → class template
- `func` + Tab → function template

**Physics:**
- `rk4` + Tab → Runge-Kutta 4th order step
- `euler` + Tab → Euler integration step
- `deriv` + Tab → Numerical derivative
- `trapz` + Tab → Trapezoidal integration
- `vec3d` + Tab → 3D vector struct
- `particle` + Tab → Particle struct

**Utility:**
- `cout` + Tab → Print to console
- `cin` + Tab → Read input
- `timer` + Tab → Benchmark timer
- `doxy` + Tab → Doxygen comment block

**Try it**: Open a .cpp file, type `main` and press Tab!

---

## 6️⃣ Testing Framework

### Setup (one-time):
```bash
brew install googletest
```

### Run Example Tests:
```bash
cd ~/Documents/CPP_Workspace
clang++ -std=c++17 tests/test_example.cpp -lgtest -lgtest_main -o bin/tests
./bin/tests
```

### Write Your Own Tests:

1. Create `tests/test_mycode.cpp`:
```cpp
#include <gtest/gtest.h>

// Your function
double square(double x) { return x * x; }

// Your test
TEST(MathTest, Square) {
    EXPECT_DOUBLE_EQ(square(2.0), 4.0);
    EXPECT_DOUBLE_EQ(square(-3.0), 9.0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

2. Compile and run:
```bash
clang++ -std=c++17 tests/test_mycode.cpp -lgtest -lgtest_main -o bin/tests
./bin/tests
```

**See `tests/README.md` for complete guide.**

---

## 8️⃣ Doxygen Documentation

### Setup (one-time):
```bash
brew install doxygen
```

### Document Your Code:
```cpp
/**
 * @brief Calculates the square of a number
 * @param x Input value
 * @return x squared
 * 
 * Example:
 * @code
 * double result = square(5.0);  // Returns 25.0
 * @endcode
 */
double square(double x) {
    return x * x;
}
```

### Generate HTML Documentation:
```bash
cd ~/Documents/CPP_Workspace
doxygen Doxyfile
open docs/doxygen/html/index.html
```

**See `docs/DOXYGEN_GUIDE.md` for complete reference.**

---

## 🎯 Recommended Workflow

### For Small Programs (like now):
1. Write code in a `.cpp` file
2. Press **F5** or click ▶️ to run
3. Use code snippets for speed

### For Larger Projects:
1. Write code with Doxygen comments
2. Use `make` to build
3. Write tests for critical functions
4. Run AddressSanitizer before releasing
5. Generate documentation with Doxygen

### When Debugging:
1. Run **Debug Build** task
2. Set breakpoints (click left of line numbers)
3. Press **F5** to start debugging
4. Use Debug panel to step through code

### For Performance Work:
1. Write code
2. Profile with **Profile Build**
3. Optimize hot spots
4. Test with **Release Build**

---

## 📁 New Directory Structure

```
CPP_Workspace/
├── .clang-format           # Code formatting rules
├── .gitignore              # Git ignore patterns
├── Doxyfile                # Documentation config
├── Makefile                # Build automation
├── main.cpp                # Your code
├── bin/                    # Compiled programs
├── tests/                  # Unit tests
│   ├── test_example.cpp
│   └── README.md
├── docs/
│   ├── SETUP_GUIDE.md
│   ├── USAGE_GUIDE.md
│   └── DOXYGEN_GUIDE.md
└── .vscode/
    ├── cpp.code-snippets   # Code snippets
    ├── tasks.json          # Build configurations
    └── launch.json
```

---

## 🆘 Quick Help

| Feature | Documentation |
|---------|---------------|
| Formatting | Automatic on save |
| Build Configs | Cmd+Shift+P → "Run Task" |
| Makefile | See `Makefile` comments |
| Snippets | Type abbreviation + Tab |
| Testing | `tests/README.md` |
| Doxygen | `docs/DOXYGEN_GUIDE.md` |

**Everything is ready to use! Start coding and explore the features as needed.**
