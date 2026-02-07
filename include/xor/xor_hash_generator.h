// Header file for XOR hash generation

#ifndef XOR_HASH_GENERATOR_H
#define XOR_HASH_GENERATOR_H

#include <vector>
#include <random>

// An XOR constraint is: x1 XOR x2 XOR ... XOR xn = bool_value
// We represent it as a set of integers (DIMACS 1-indexed) XORed together equal to a boolean value
struct XORConstraint {
    std::vector<int> variables;
    bool value;
    
    XORConstraint() : value(false) {}
    XORConstraint(const std::vector<int>& vars, bool v) : variables(vars), value(v) {}
    
    size_t size() const { return variables.size(); }
    bool empty() const { return variables.empty(); }
};

class XORHashGenerator {
public:
    // Generate a single sparse XOR constraint
    // numVariables: total number of variables in the formula
    // density: probability that each variable appears in the XOR
    //     NOTE: this is set as a default value for now - I will use ML to improve on this in the future by predicting individual variable inclusion probabilities
    static XORConstraint generateSparseXOR(int numVariables, double density = 0.1);
    
    // Generate multiple XOR constraints
    static std::vector<XORConstraint> generateXORFamily(int numVariables, int numXORs, double density = 0.1);
    
    // Set random seed for reproducibility
    static void setSeed(unsigned int seed);
    
private:
    static std::mt19937 rng;
};

#endif // XOR_HASH_GENERATOR_H
