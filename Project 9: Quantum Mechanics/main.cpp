#include <cstdlib>
#include <iostream>
#include <vector>

#include "processing copy.h"

int main() {

    const int maxN = 6;
    const int nodesToFind = 10;

    EigenstateFinder finder(maxN, nodesToFind);
    std::map<int, std::vector<Eigenstate>> eigenstatesMap = finder.findEigenstatesForAllN(0.01, 0.05);
    outHelper outputHelper("output");
    outputHelper.saveEigenstates(eigenstatesMap);

    // ESweep sweep(n);

    // const double E_min = 0.1;
    // const double E_step = 0.005;

    // std::vector<NodalBracket> brackets = sweep.findNodalBrackets(E_min, E_step);

    // std::cout << "First 10 nodal brackets for n=2:\n";
    // for (size_t i = 0; i < brackets.size() && i < 10; ++i) {
    //     const NodalBracket &b = brackets[i];
    //     std::cout << "node=" << b.node << "  ["
    //               << b.minusEnergy << ", " << b.plusEnergy << "]\n";
    // }



    return 0;
}