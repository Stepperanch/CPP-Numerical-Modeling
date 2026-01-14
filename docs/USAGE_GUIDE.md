# C++ Development Environment - Usage Guide

## ⌨️ Keyboard Shortcuts

### Primary Shortcuts
| Shortcut | Action | Description |
|----------|--------|-------------|
| **F5** | Build + Run | Compile and execute current file |
| **Cmd+Shift+B** | Build Only | Compile without running |
| **Cmd+K Cmd+S** | Keyboard Shortcuts | View all VS Code shortcuts |
| **Ctrl+C** | Stop Program | Terminate running program in terminal |

### Navigation
| Shortcut | Action |
|----------|--------|
| **Cmd+P** | Quick file open |
| **Cmd+Shift+F** | Search in all files |
| **Cmd+B** | Toggle sidebar |
| **Ctrl+`** | Toggle terminal |
| **Cmd+Shift+E** | Explorer view |

### Editing
| Shortcut | Action |
|----------|--------|
| **Cmd+D** | Select next occurrence |
| **Cmd+Shift+L** | Select all occurrences |
| **Cmd+/** | Toggle line comment |
| **Cmd+Shift+[** | Fold code block |
| **Cmd+Shift+]** | Unfold code block |
| **Option+Up/Down** | Move line up/down |
| **Shift+Option+Up/Down** | Copy line up/down |

### IntelliSense
| Shortcut | Action |
|----------|--------|
| **Ctrl+Space** | Trigger autocomplete |
| **Cmd+.** | Quick fix suggestions |
| **F12** | Go to definition |
| **Shift+F12** | Show all references |
| **Option+F12** | Peek definition |

---

## 🎮 Workflow Examples

### Example 1: Creating a Simple Program
```cpp
// 1. Create file: physics_sim.cpp
// 2. Write code:

#include <iostream>
#include <cmath>

int main() {
    double velocity = 9.8;  // m/s
    double time = 2.0;      // seconds
    double distance = velocity * time + 0.5 * 9.8 * time * time;
    
    std::cout << "Distance traveled: " << distance << " meters" << std::endl;
    return 0;
}

// 3. Press F5
// 4. See output in terminal
```

### Example 2: Debugging with Print Statements
```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    
    // Debug: Check vector contents
    std::cout << "Debug: Vector size = " << numbers.size() << std::endl;
    
    for (size_t i = 0; i < numbers.size(); ++i) {
        std::cout << "Debug: numbers[" << i << "] = " << numbers[i] << std::endl;
    }
    
    return 0;
}
```

### Example 3: Working with Multiple Files
```cpp
// calculator.h
#ifndef CALCULATOR_H
#define CALCULATOR_H

double add(double a, double b);
double multiply(double a, double b);

#endif

// calculator.cpp
#include "calculator.h"

double add(double a, double b) {
    return a + b;
}

double multiply(double a, double b) {
    return a * b;
}

// main.cpp
#include <iostream>
#include "calculator.h"

int main() {
    std::cout << "5 + 3 = " << add(5, 3) << std::endl;
    std::cout << "5 * 3 = " << multiply(5, 3) << std::endl;
    return 0;
}

// To compile multiple files, modify tasks.json:
// Replace: "${file}"
// With: "${workspaceFolder}/*.cpp"
```

---

## 🏃 Quick Reference: Common Tasks

### Read User Input
```cpp
#include <iostream>
#include <string>

int main() {
    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    std::cout << "Hello, " << name << "!" << std::endl;
    return 0;
}
```

### File I/O
```cpp
#include <iostream>
#include <fstream>
#include <string>

int main() {
    // Write to file
    std::ofstream outFile("output.txt");
    outFile << "Hello, File!" << std::endl;
    outFile.close();
    
    // Read from file
    std::ifstream inFile("output.txt");
    std::string line;
    while (std::getline(inFile, line)) {
        std::cout << line << std::endl;
    }
    inFile.close();
    
    return 0;
}
```

### Command Line Arguments
```cpp
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Number of arguments: " << argc << std::endl;
    
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
    }
    
    return 0;
}

// Run with: ./bin/program arg1 arg2 arg3
```

### Timing Code Execution
```cpp
#include <iostream>
#include <chrono>
#include <cmath>

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Code to measure
    double sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += std::sqrt(i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Result: " << sum << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    
    return 0;
}
```

---

## 🔥 Advanced Features

### Using Custom Build Tasks

Press **Cmd+Shift+P** → Type "Tasks: Run Task" → Select:
- **Build C++**: Standard compilation
- **Build and Run**: F5 equivalent
- **Clean Build**: Remove all binaries

### Creating Your Own Task
1. Open `.vscode/tasks.json`
2. Add new task in the `tasks` array:
```json
{
    "label": "Build Release",
    "type": "shell",
    "command": "clang++",
    "args": [
        "-std=c++17",
        "-O3",  // Maximum optimization
        "-DNDEBUG",  // Define NDEBUG for release
        "${file}",
        "-o",
        "${fileDirname}/bin/${fileBasenameNoExtension}_release"
    ],
    "group": "build",
    "problemMatcher": ["$gcc"]
}
```

### Switching C++ Standards
Quick change without editing config:
```bash
# In terminal:
clang++ -std=c++20 -O2 main.cpp -o bin/main
```

---

## 📊 Output Formatting

### Clean Console Output
```cpp
#include <iostream>
#include <iomanip>

int main() {
    double pi = 3.14159265359;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Pi (2 decimals): " << pi << std::endl;
    
    std::cout << std::setprecision(6);
    std::cout << "Pi (6 decimals): " << pi << std::endl;
    
    // Table formatting
    std::cout << std::setw(10) << "Number" 
              << std::setw(10) << "Square" << std::endl;
    std::cout << "--------------------" << std::endl;
    
    for (int i = 1; i <= 5; ++i) {
        std::cout << std::setw(10) << i 
                  << std::setw(10) << i*i << std::endl;
    }
    
    return 0;
}
```

---

## 🎯 Best Practices

### 1. **File Organization**
```
CPP_Workspace/
├── main.cpp           # Entry point
├── include/           # Header files
│   └── myclass.h
├── src/               # Source files
│   └── myclass.cpp
└── bin/               # Executables
```

### 2. **Naming Conventions**
- **Files**: lowercase with underscores (`my_program.cpp`)
- **Classes**: PascalCase (`MyClass`)
- **Functions**: camelCase (`calculateDistance`)
- **Constants**: UPPER_CASE (`MAX_SIZE`)

### 3. **Code Comments**
```cpp
// Single-line comment for brief notes

/*
 * Multi-line comment for detailed explanations
 * Use for function headers and complex logic
 */

/**
 * Documentation comment (Doxygen style)
 * @param x The input value
 * @return The computed result
 */
double myFunction(double x) {
    return x * 2.0;
}
```

### 4. **Error Handling**
```cpp
#include <iostream>
#include <stdexcept>

double safeDivide(double a, double b) {
    if (b == 0) {
        throw std::runtime_error("Division by zero!");
    }
    return a / b;
}

int main() {
    try {
        double result = safeDivide(10, 0);
        std::cout << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
```

---

## 🚨 Common Mistakes to Avoid

1. **Forgetting return statement in main()**
   ```cpp
   int main() {
       // code here
       return 0;  // Don't forget!
   }
   ```

2. **Not closing files**
   ```cpp
   std::ofstream file("data.txt");
   file << "data";
   file.close();  // Always close!
   ```

3. **Integer division**
   ```cpp
   int a = 5, b = 2;
   std::cout << a / b << std::endl;        // Output: 2
   std::cout << (double)a / b << std::endl; // Output: 2.5
   ```

4. **Array bounds**
   ```cpp
   int arr[5] = {1, 2, 3, 4, 5};
   // arr[5] is out of bounds! Use arr[4] for last element
   ```

---

## 📈 Performance Monitoring

### Memory Usage
```bash
# In terminal after running:
time ./bin/main
```

### Profile with Instruments (macOS)
```bash
# Build with profiling enabled:
clang++ -std=c++17 -O2 -g -pg main.cpp -o bin/main

# Run and generate profile:
./bin/main
gprof bin/main gmon.out > analysis.txt
```

---

## 🔧 Troubleshooting Quick Fixes

| Problem | Solution |
|---------|----------|
| "Undefined reference" | Link required library with `-l` flag |
| "No such file" | Check file path, use absolute paths |
| Slow compilation | Reduce optimization level (use `-O0`) |
| Segmentation fault | Check array bounds, pointer validity |
| "Permission denied" | Run `chmod +x bin/program` |

---

## 🎓 Next Steps

1. Explore `reference/CPP_BASICS.md` for syntax review
2. Try `reference/NUMERICAL_PHYSICS.md` for physics simulations
3. Check `reference/OPTIMIZATION_TIPS.md` for performance tuning
4. Experiment with the standard library (vectors, algorithms, etc.)

**Happy Coding! 🚀**
