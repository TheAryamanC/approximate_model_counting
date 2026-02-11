// Header file for CNF simplification

#ifndef CNF_SIMPLIFIER_H
#define CNF_SIMPLIFIER_H

#include <vector>
#include <unordered_map>
#include "cnf/cnf_structure.h"
#include "solver/partial_assignment.h"

// Apply partial assignment to CNF
struct SimplificationResult {
    CNFFormula simplified;
    bool isUnsatisfiable;
    bool isTriviallyTrue;
    int clausesRemoved;
    int literalsRemoved;
    
    SimplificationResult() : 
        isUnsatisfiable(false), 
        isTriviallyTrue(false), 
        clausesRemoved(0), 
        literalsRemoved(0) {}
};

class CNFSimplifier {
public:
    // Apply partial assignment to simplify the CNF formula
    static SimplificationResult applyAssignment(const CNFFormula& formula, const std::unordered_map<int, int>& assignment);
    
    // Apply XOR solution result to simplify CNF
    static SimplificationResult applyXORSolution(const CNFFormula& formula, const XORSolutionResult& xorSolution);
    
    // Check if a literal is satisfied by the assignment
    static bool isLiteralSatisfied(Literal lit, const std::unordered_map<int, int>& assignment);
    
    // Check if a literal is falsified by the assignment
    static bool isLiteralFalsified(Literal lit, const std::unordered_map<int, int>& assignment);
};

#endif // CNF_SIMPLIFIER_H
