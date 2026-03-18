#include <cstdlib>
#include <iostream>
#include <vector>

#include "processing.h"

int main() {
    // n = 2 corresponds to the harmonic potential well V(x) = x^2 / 2.
    constexpr int n = 2;
    constexpr int bracketsToFind = 10;

    ESweep sweep(n, bracketsToFind);
    sweep.x0 = 0.0;
    sweep.xEnd = 8.0;

    const double E_min = 0.1;
    const double E_step = 0.05;
    const int trajectorySteps = 4000;

    const std::vector<NodalBracket> brackets =
        sweep.findNodalBrackets(E_min, E_step, 20.0, trajectorySteps);

    if (brackets.size() < static_cast<size_t>(bracketsToFind)) {
        std::cerr << "Failed to find " << bracketsToFind
                  << " nodal brackets. Found: " << brackets.size() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "First " << bracketsToFind << " nodal brackets for n=4:\n";
    for (int i = 0; i < bracketsToFind; ++i) {
        const NodalBracket &b = brackets[static_cast<size_t>(i)];

        // Basic consistency checks for each bracket.
        if (!(b.plusEnergy > b.minusEnergy)) {
            std::cerr << "Invalid bracket at index " << i
                      << ": plusEnergy must be greater than minusEnergy.\n";
            return EXIT_FAILURE;
        }
        if (b.node != i + 1) {
            std::cerr << "Unexpected node label at index " << i
                      << ": expected " << (i + 1) << ", got " << b.node << '\n';
            return EXIT_FAILURE;
        }

        std::cout << "node=" << b.node << "  ["
                  << b.minusEnergy << ", " << b.plusEnergy << "]\n";
    }

    std::cout << "Test passed.\n";
    return EXIT_SUCCESS;
}
