// Header file for partial assignments

#ifndef PARTIAL_ASSIGNMENT_H
#define PARTIAL_ASSIGNMENT_H

#include <vector>
#include <unordered_map>
#include "xor/xor_hash_generator.h"

// Value of a variable in the partial assignment
enum AssignmentValue {
    FALSE_VAL = 0,
    TRUE_VAL = 1,
    UNASSIGNED = -1
};

// Result of solving XOR constraints
struct XORSolutionResult {
    bool satisfiable;
    std::unordered_map<int, int> assignment;
    std::vector<int> freeVariables;
    
    XORSolutionResult() : satisfiable(true) {}
};

class PartialAssignment {
public:
    // Solve a system of XOR constraints using Gaussian elimination
    // Returns a partial assignment or indicates unsatisfiability
    static XORSolutionResult solveXORSystem(const std::vector<XORConstraint>& xors, int numVariables);

private:
    // Gaussian elimination for XOR constraints
    static XORSolutionResult gaussianElimination(std::vector<std::vector<int>>& matrix, std::vector<int>& rhs, int numVariables);
};

#endif // PARTIAL_ASSIGNMENT_H
