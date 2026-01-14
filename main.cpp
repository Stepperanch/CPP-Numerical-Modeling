/*
 * Hello World - C++ Development Environment Test
 * 
 * Build and Run: Press F5
 * Build Only: Cmd+Shift+B
 * 
 * Compiler: clang++ (C++17)
 * Optimizations: -O2 -Wall -Wextra
 */

#include <iostream>
#include <string>
#include <cmath>

int main() {
    // Basic output test
    std::cout << "======================================" << std::endl;
    std::cout << "  C++ Development Environment Ready!" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;
    
    // String handling test
    std::string name = "CPP_Workspace";
    std::cout << "Workspace: " << name << std::endl;
    std::cout << "Compiler: Clang++ 17.0.0" << std::endl;
    std::cout << "Standard: C++17" << std::endl;
    std::cout << std::endl;
    
    // Math library test
    double pi = std::acos(-1.0);
    std::cout << "Math Library Test:" << std::endl;
    std::cout << "  π = " << pi << std::endl;
    std::cout << "  sin(π/2) = " << std::sin(pi/2.0) << std::endl;
    std::cout << "  √2 = " << std::sqrt(2.0) << std::endl;
    std::cout << std::endl;
    
    // Loop test
    std::cout << "Loop Test (Fibonacci sequence):" << std::endl;
    int a = 0, b = 1;
    std::cout << "  ";
    for (int i = 0; i < 10; ++i) {
        std::cout << a << " ";
        int temp = a + b;
        a = b;
        b = temp;
    }
    std::cout << std::endl << std::endl;
    
    std::cout << "✓ All systems operational!" << std::endl;
    std::cout << "✓ Press F5 anytime to rebuild and run" << std::endl;
    
    return 0;
}
