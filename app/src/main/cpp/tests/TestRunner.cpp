#include <iostream>
#include "GraphTests.h"
#include "ResponseTests.h"

/**
 * Dedicated native test executable entry point.
 */
int main(int argc, char** argv) {
    std::cout << "Starting PowerEQ Native Tests..." << std::endl;

    uint32_t failures = GraphTests::runAll();

    std::cout << "Starting Response Math Tests..." << std::endl;
    if (!ResponseTests::runAll()) {
        failures++;
    }

    if (failures == 0) {
        std::cout << "All tests PASSED." << std::endl;
        return 0;
    } else {
        std::cerr << "Tests FAILED with " << failures << " failures." << std::endl;
        return 1;
    }
}
