// Source file for XOR hash generator implementation

#include "xor/xor_hash_generator.h"
#include <algorithm>
#include <chrono>

using namespace std;

// random number generator
mt19937 XORHashGenerator::rng(chrono::steady_clock::now().time_since_epoch().count());

void XORHashGenerator::setSeed(unsigned int seed) {
    rng.seed(seed);
}

XORConstraint XORHashGenerator::generateSparseXOR(int numVariables, double density) {
    XORConstraint xor_constraint;
    uniform_real_distribution<double> dist(0.0, 1.0);
    
    // add to XOR constraint with probability = density
    for (int i = 1; i <= numVariables; ++i) {
        if (dist(rng) < density) {
            xor_constraint.variables.push_back(i);
        }
    }
    
    // randomly assign value
    uniform_int_distribution<int> value(0, 1);
    xor_constraint.value = value(rng);
    
    return xor_constraint;
}

vector<XORConstraint> XORHashGenerator::generateXORFamily(int numVariables, int numXORs, double density) {
    vector<XORConstraint> xors;
    xors.reserve(numXORs);
    
    for (int i = 0; i < numXORs; ++i) {
        xors.push_back(generateSparseXOR(numVariables, density));
    }
    
    return xors;
}
