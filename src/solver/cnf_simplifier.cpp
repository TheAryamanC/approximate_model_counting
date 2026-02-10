// Source file for CNF simplification

#include "solver/cnf_simplifier.h"
#include <iostream>
#include <cmath>

using namespace std;

// check if literal is satisfied by assignment
bool CNFSimplifier::isLiteralSatisfied(Literal lit, const unordered_map<int, int>& assignment) {
    int var = abs(lit);
    
    // check if assigned
    auto it = assignment.find(var);
    if (it == assignment.end()) {
        return false;
    }
    
    int value = it->second;
    
    // positive literal satisfied if variable = 1, negative literal satisfied if variable = 0
    if (lit > 0) {
        return value == 1;
    } else {
        return value == 0;
    }
}

// check if literal is falsified by assignment
bool CNFSimplifier::isLiteralFalsified(Literal lit, const unordered_map<int, int>& assignment) {
    int var = abs(lit);
    
    // check if assigned
    auto it = assignment.find(var);
    if (it == assignment.end()) {
        return false;
    }
    
    int value = it->second;
    
    // positive literal falsified if variable = 0, negative literal falsified if variable = 1
    if (lit > 0) {
        return value == 0;
    } else {
        return value == 1;
    }
}

// apply partial assignment to CNF formula
SimplificationResult CNFSimplifier::applyAssignment(const CNFFormula& formula, const unordered_map<int, int>& assignment) {
    SimplificationResult result;
    result.simplified = CNFFormula(formula.numVariables, 0);
    
    // process each clause
    for (const auto& clause : formula.clauses) {
        bool clauseSatisfied = false;
        Clause simplifiedClause;
        
        // check each literal in clause
        for (Literal lit : clause.literals) {
            if (isLiteralSatisfied(lit, assignment)) {
                // if any literal is satisfied, entire clause is satisfied
                clauseSatisfied = true;
                break;
            } else if (isLiteralFalsified(lit, assignment)) {
                // this literal is false, remove it from clause
                result.literalsRemoved++;
                // don't add to simplified clause
            } else {
                // unassigned, keep it
                simplifiedClause.addLiteral(lit);
            }
        }
        
        if (clauseSatisfied) {
            // clause is satisfied, don't add to simplified formula
            result.clausesRemoved++;
        } else {
            // clause is not satisfied, add simplified version
            if (simplifiedClause.empty()) {
                result.isUnsatisfiable = true; // empty clause is unsatisfiable
                return result;
            }
            result.simplified.addClause(simplifiedClause);
        }
    }
    
    // update clause count
    result.simplified.numClauses = result.simplified.clauses.size();
    
    // check if formula is trivially true (all clauses satisfied)
    if (result.simplified.clauses.empty()) {
        result.isTriviallyTrue = true;
    }
    
    return result;
}

// apply XOR solution to simplify CNF formula
SimplificationResult CNFSimplifier::applyXORSolution(const CNFFormula& formula, const XORSolutionResult& xorSolution) {
    if (!xorSolution.satisfiable) {
        SimplificationResult result;
        result.isUnsatisfiable = true; // XOR system is unsatisfiable, so formula is unsatisfiable
        return result;
    }
    
    // apply the XOR assignment to the CNF
    return applyAssignment(formula, xorSolution.assignment);
}
